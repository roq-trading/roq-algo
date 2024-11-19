/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/reporter/summary.hpp"

#include <cassert>
#include <vector>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/enum.hpp"
#include "roq/utils/safe_cast.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/container.hpp"

#include "roq/algo/tools/market_data.hpp"
#include "roq/algo/tools/position_tracker.hpp"
#include "roq/algo/tools/time_checker.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace reporter {

// === CONSTANTS ===

namespace {
auto const DEFAULT_CONFIG = Summary::Config{
    .market_data_source = MarketDataSource::TOP_OF_BOOK,
    .sample_frequency = 1min,
};
}

// === HELPERS ===

namespace {
struct Implementation final : public Reporter {
  explicit Implementation(Summary::Config const &config) : market_data_source_{config.market_data_source}, sample_frequency_{config.sample_frequency} {}

 protected:
  struct Instrument final {
    Instrument(std::string_view const &exchange, std::string_view const &symbol, MarketDataSource market_data_source)
        : market_data{exchange, symbol, market_data_source} {}

    bool operator()(Event<OrderUpdate> const &event) {
      auto &[message_info, order_update] = event;
      // orders
      auto order = Order{
          .order_id = order_update.order_id,
          .side = order_update.side,
          .create_time_utc = order_update.create_time_utc,
          .update_time_utc = order_update.update_time_utc,
          .order_status = order_update.order_status,
          .quantity = order_update.quantity,
          .price = order_update.price,
          .remaining_quantity = order_update.remaining_quantity,
          .traded_quantity = order_update.traded_quantity,
          .average_traded_price = order_update.average_traded_price,
          .sending_time_utc = order_update.sending_time_utc,
      };
      orders[std::string{order_update.account}].emplace_back(std::move(order));
      return true;
    }

    bool operator()(Event<TradeUpdate> const &event) {
      position_tracker(event);
      auto &[message_info, trade_update_2] = event;
      for (auto &fill : trade_update_2.fills) {
        assert(utils::compare(fill.quantity, 0.0) > 0);
        switch (trade_update_2.side) {
          using enum Side;
          case UNDEFINED:
            assert(false);
            break;
          case BUY:
            ++trade_update.fills.buy_count;
            trade_update.fills.buy_volume += fill.quantity;
            break;
          case SELL:
            ++trade_update.fills.sell_count;
            trade_update.fills.sell_volume += fill.quantity;
            break;
        }
        ++trade_update.fills.total_count;
        trade_update.fills.total_volume += fill.quantity;
        // trades
        auto fill_2 = Fill{
            .order_id = trade_update_2.order_id,
            .side = trade_update_2.side,
            .create_time_utc = trade_update_2.create_time_utc,
            .update_time_utc = trade_update_2.update_time_utc,
            .exchange_time_utc = fill.exchange_time_utc,
            .external_trade_id = std::string{fill.external_trade_id},
            .quantity = fill.quantity,
            .price = fill.price,
            .liquidity = fill.liquidity,
        };
        fills[std::string{trade_update_2.account}].emplace_back(std::move(fill_2));
      }
      return true;
    }

    bool operator()(Event<PositionUpdate> const &event) {
      position_tracker(event);
      auto &[message_info, position_update_2] = event;
      ++position_update.total_count;
      auto position = position_update_2.long_quantity - position_update_2.short_quantity;
      utils::update_min(position_update.position_min, position);
      utils::update_max(position_update.position_max, position);
      return true;
    }

    tools::MarketData market_data;
    tools::PositionTracker position_tracker;

    // market data
    struct ReferenceData final {
      size_t total_count = {};
    } reference_data;
    struct MarketStatus final {
      size_t total_count = {};
    } market_status;
    struct TopOfBook final {
      size_t total_count = {};
    } top_of_book;
    struct MarketByPriceUpdate final {
      size_t total_count = {};
    } market_by_price_update;
    struct MarketByOrderUpdate final {
      size_t total_count = {};
    } market_by_order_update;
    struct TradeSummary final {
      size_t total_count = {};
    } trade_summary;
    struct StatisticsUpdate final {
      size_t total_count = {};
    } statistics_update;
    // order management
    struct OrderAck final {
      size_t accepted_count = {};
      size_t rejected_count = {};
    } order_ack;
    struct OrderUpdate final {
      size_t buy_count = {};
      size_t sell_count = {};
      size_t total_count = {};
    } order_update;
    struct TradeUpdate final {
      struct Fills final {
        size_t buy_count = {};
        size_t sell_count = {};
        size_t total_count = {};
        double buy_volume = 0.0;
        double sell_volume = 0.0;
        double total_volume = 0.0;
      } fills;
    } trade_update;
    struct PositionUpdate final {
      size_t total_count = {};
      double position_min = NaN;
      double position_max = NaN;
    } position_update;
    // samples
    struct Sample final {
      double best_bid_price = NaN;
      double best_ask_price = NaN;
      double buy_volume = NaN;
      double sell_volume = NaN;
      double position = NaN;
      double average_price = NaN;
      double mark_price = NaN;
      double unrealized_profit = NaN;
      double realized_profit = NaN;
    };
    std::vector<std::pair<std::chrono::nanoseconds, Sample>> history;
    // orders
    struct Order final {
      uint64_t order_id = {};
      Side side = {};
      std::chrono::nanoseconds create_time_utc = {};
      std::chrono::nanoseconds update_time_utc = {};
      OrderStatus order_status = {};
      double quantity = NaN;
      double price = NaN;
      double remaining_quantity = NaN;
      double traded_quantity = NaN;
      double average_traded_price = NaN;
      std::chrono::nanoseconds sending_time_utc = {};
    };
    std::unordered_map<std::string, std::vector<Order>> orders;  // by account
    // trades
    struct Fill final {
      uint64_t order_id = {};
      Side side = {};
      std::chrono::nanoseconds create_time_utc = {};
      std::chrono::nanoseconds update_time_utc = {};
      std::chrono::nanoseconds exchange_time_utc = {};
      std::string external_trade_id;
      double quantity = NaN;
      double price = NaN;
      Liquidity liquidity = {};
    };
    std::unordered_map<std::string, std::vector<Fill>> fills;  // by account
  };

  // reporter

  std::span<std::string_view const> get_labels() const override {
    // XXX FIXME autogen from magic_enum::enum_names
    static std::array<std::string_view const, 3> const RESULT{{
        "sample_history",
        "order_update",
        "trade_update",
    }};
    return RESULT;
  }

  void dispatch(Handler &handler, std::string_view const &label) const override {
    enum class Label {
      SAMPLE_HISTORY,
      ORDER_UPDATE,
      TRADE_UPDATE,
    };
    auto label_2 = utils::parse_enum<Label>(label);
    switch (label_2) {
      using enum Label;
      case SAMPLE_HISTORY:
        dispatch_sample_history(handler);
        break;
      case ORDER_UPDATE:
        dispatch_order_update(handler);
        break;
      case TRADE_UPDATE:
        dispatch_trade_update(handler);
        break;
    }
  }

  void dispatch_sample_history(Handler &handler) const {
    std::vector<uint8_t> source_2;
    std::vector<std::string_view> exchange_2, symbol_2;
    std::vector<std::chrono::nanoseconds> sample_period_utc_2;
    std::vector<double> best_bid_price, best_ask_price, buy_volume, sell_volume, position, average_price, mark_price, unrealized_profit, realized_profit;
    for (size_t source = 0; source < std::size(instruments_); ++source) {
      auto &tmp_1 = instruments_[source];
      for (auto &[exchange, tmp_2] : tmp_1) {
        for (auto &[symbol, instrument] : tmp_2) {
          for (auto &[sample_period_utc, sample] : instrument.history) {
            source_2.emplace_back(source);
            exchange_2.emplace_back(exchange);
            symbol_2.emplace_back(symbol);
            sample_period_utc_2.emplace_back(sample_period_utc);
            best_bid_price.emplace_back(sample.best_bid_price);
            best_ask_price.emplace_back(sample.best_ask_price);
            buy_volume.emplace_back(sample.buy_volume);
            sell_volume.emplace_back(sample.sell_volume);
            position.emplace_back(sample.position);
            average_price.emplace_back(sample.average_price);
            mark_price.emplace_back(sample.mark_price);
            unrealized_profit.emplace_back(sample.unrealized_profit);
            realized_profit.emplace_back(sample.realized_profit);
          }
        }
      }
    }
    handler("source"sv, roq::algo::Reporter::Type::INDEX, source_2);
    handler("exchange"sv, roq::algo::Reporter::Type::INDEX, exchange_2);
    handler("symbol"sv, roq::algo::Reporter::Type::INDEX, symbol_2);
    handler("sample_period_utc"sv, roq::algo::Reporter::Type::INDEX, sample_period_utc_2);
    handler("best_bid_price"sv, roq::algo::Reporter::Type::DATA, best_bid_price);
    handler("best_ask_price"sv, roq::algo::Reporter::Type::DATA, best_ask_price);
    handler("buy_volume"sv, roq::algo::Reporter::Type::DATA, buy_volume);
    handler("sell_volume"sv, roq::algo::Reporter::Type::DATA, sell_volume);
    handler("position"sv, roq::algo::Reporter::Type::DATA, position);
    handler("average_price"sv, roq::algo::Reporter::Type::DATA, average_price);
    handler("mark_price"sv, roq::algo::Reporter::Type::DATA, mark_price);
    handler("unrealized_profit"sv, roq::algo::Reporter::Type::DATA, unrealized_profit);
    handler("realized_profit"sv, roq::algo::Reporter::Type::DATA, realized_profit);
  }

  void dispatch_order_update(Handler &handler) const {
    std::vector<uint8_t> source_2;
    std::vector<uint64_t> order_id_2;
    std::vector<std::string_view> exchange_2, symbol_2, account_2, side, order_status;
    std::vector<std::chrono::nanoseconds> create_time_utc, update_time_utc, sending_time_utc;
    std::vector<double> quantity, price, remaining_quantity, traded_quantity, average_traded_price;
    for (size_t source = 0; source < std::size(instruments_); ++source) {
      auto &tmp_1 = instruments_[source];
      for (auto &[exchange, tmp_2] : tmp_1) {
        for (auto &[symbol, instrument] : tmp_2) {
          for (auto &[account, orders_2] : instrument.orders) {
            for (auto &order : orders_2) {
              source_2.emplace_back(source);
              exchange_2.emplace_back(exchange);
              symbol_2.emplace_back(symbol);
              account_2.emplace_back(account);
              order_id_2.emplace_back(order.order_id);
              side.emplace_back(magic_enum::enum_name(order.side));
              create_time_utc.emplace_back(order.create_time_utc);
              update_time_utc.emplace_back(order.update_time_utc);
              order_status.emplace_back(magic_enum::enum_name(order.order_status));
              quantity.emplace_back(order.quantity);
              price.emplace_back(order.price);
              remaining_quantity.emplace_back(order.remaining_quantity);
              traded_quantity.emplace_back(order.traded_quantity);
              average_traded_price.emplace_back(order.average_traded_price);
              sending_time_utc.emplace_back(order.sending_time_utc);
            }
          }
        }
      }
    }
    handler("source"sv, roq::algo::Reporter::Type::INDEX, source_2);
    handler("exchange"sv, roq::algo::Reporter::Type::INDEX, exchange_2);
    handler("symbol"sv, roq::algo::Reporter::Type::INDEX, symbol_2);
    handler("account"sv, roq::algo::Reporter::Type::INDEX, account_2);
    handler("order_id"sv, roq::algo::Reporter::Type::INDEX, order_id_2);
    handler("side"sv, roq::algo::Reporter::Type::DATA, side);
    handler("create_time_utc"sv, roq::algo::Reporter::Type::DATA, create_time_utc);
    handler("update_time_utc"sv, roq::algo::Reporter::Type::DATA, update_time_utc);
    handler("order_status"sv, roq::algo::Reporter::Type::DATA, order_status);
    handler("quantity"sv, roq::algo::Reporter::Type::DATA, quantity);
    handler("price"sv, roq::algo::Reporter::Type::DATA, price);
    handler("remaining_quantity"sv, roq::algo::Reporter::Type::DATA, remaining_quantity);
    handler("traded_quantity"sv, roq::algo::Reporter::Type::DATA, traded_quantity);
    handler("average_traded_price"sv, roq::algo::Reporter::Type::DATA, average_traded_price);
    handler("sending_time_utc"sv, roq::algo::Reporter::Type::INDEX, sending_time_utc);
  }

  void dispatch_trade_update(Handler &handler) const {
    std::vector<uint8_t> source_2;
    std::vector<uint64_t> order_id;
    std::vector<std::string_view> exchange_2, symbol_2, account_2, side, external_trade_id, liquidity;
    std::vector<std::chrono::nanoseconds> exchange_time_utc, create_time_utc, update_time_utc;
    std::vector<double> quantity, price;
    for (size_t source = 0; source < std::size(instruments_); ++source) {
      auto &tmp_1 = instruments_[source];
      for (auto &[exchange, tmp_2] : tmp_1) {
        for (auto &[symbol, instrument] : tmp_2) {
          for (auto &[account, fills] : instrument.fills) {
            for (auto &fill : fills) {
              source_2.emplace_back(source);
              exchange_2.emplace_back(exchange);
              symbol_2.emplace_back(symbol);
              account_2.emplace_back(account);
              order_id.emplace_back(fill.order_id);
              side.emplace_back(magic_enum::enum_name(fill.side));
              create_time_utc.emplace_back(fill.create_time_utc);
              update_time_utc.emplace_back(fill.update_time_utc);
              exchange_time_utc.emplace_back(fill.exchange_time_utc);
              external_trade_id.emplace_back(fill.external_trade_id);
              quantity.emplace_back(fill.quantity);
              price.emplace_back(fill.price);
              liquidity.emplace_back(magic_enum::enum_name(fill.liquidity));
            }
          }
        }
      }
    }
    handler("source"sv, roq::algo::Reporter::Type::INDEX, source_2);
    handler("exchange"sv, roq::algo::Reporter::Type::INDEX, exchange_2);
    handler("symbol"sv, roq::algo::Reporter::Type::INDEX, symbol_2);
    handler("account"sv, roq::algo::Reporter::Type::INDEX, account_2);
    handler("order_id"sv, roq::algo::Reporter::Type::INDEX, order_id);
    handler("side"sv, roq::algo::Reporter::Type::DATA, side);
    handler("create_time_utc"sv, roq::algo::Reporter::Type::DATA, create_time_utc);
    handler("update_time_utc"sv, roq::algo::Reporter::Type::DATA, update_time_utc);
    handler("exchange_time_utc"sv, roq::algo::Reporter::Type::INDEX, exchange_time_utc);
    handler("external_trade_id"sv, roq::algo::Reporter::Type::INDEX, external_trade_id);
    handler("quantity"sv, roq::algo::Reporter::Type::DATA, quantity);
    handler("price"sv, roq::algo::Reporter::Type::DATA, price);
    handler("liquidity"sv, roq::algo::Reporter::Type::DATA, liquidity);
  }

  void print(OutputType output_type, std::string_view const &label) const override {
    switch (output_type) {
      using enum OutputType;
      case TEXT:
        if (!std::empty(label))
          throw RuntimeError{R"(Unexpected: label="{}")"sv, label};
        print_text();
        break;
      case JSON:
        print_json(label);
        break;
      case CSV:
        if (label != "history"sv)
          throw RuntimeError{R"(Unexpected: label="{}")"sv, label};
        print_csv_history();
        break;
    }
  }

  void print_text() const {
    for (size_t source = 0; source < std::size(instruments_); ++source) {
      print_helper(0, "source"sv, source);
      auto &tmp_1 = instruments_[source];
      for (auto &[exchange, tmp_2] : tmp_1) {
        print_helper(2, "exchange"sv, exchange);
        for (auto &[symbol, instrument] : tmp_2) {
          print_helper(4, "symbol"sv, symbol);
          print_helper(6, "market_data"sv);
          print_helper(8, "reference_data"sv);
          print_helper(10, "total_count"sv, instrument.reference_data.total_count);
          print_helper(8, "market_status"sv);
          print_helper(10, "total_count"sv, instrument.market_status.total_count);
          print_helper(8, "top_of_book"sv);
          print_helper(10, "total_count"sv, instrument.top_of_book.total_count);
          print_helper(8, "market_by_price_update"sv);
          print_helper(10, "total_count"sv, instrument.market_by_price_update.total_count);
          print_helper(8, "market_by_order_update"sv);
          print_helper(10, "total_count"sv, instrument.market_by_order_update.total_count);
          print_helper(8, "trade_summary"sv);
          print_helper(10, "total_count"sv, instrument.trade_summary.total_count);
          print_helper(8, "statistics_update"sv);
          print_helper(10, "total_count"sv, instrument.statistics_update.total_count);
          print_helper(6, "order_management"sv);
          print_helper(8, "order_ack"sv);
          print_helper(10, "accepted_count"sv, instrument.order_ack.accepted_count);
          print_helper(10, "rejected_count"sv, instrument.order_ack.rejected_count);
          print_helper(8, "order_update"sv);
          print_helper(10, "buy_count"sv, instrument.order_update.buy_count);
          print_helper(10, "sell_count"sv, instrument.order_update.sell_count);
          print_helper(10, "total_count"sv, instrument.order_update.total_count);
          print_helper(8, "trade_update"sv);
          print_helper(10, "fills"sv);
          print_helper(12, "buy_count"sv, instrument.trade_update.fills.buy_count);
          print_helper(12, "sell_count"sv, instrument.trade_update.fills.sell_count);
          print_helper(12, "total_count"sv, instrument.trade_update.fills.total_count);
          print_helper(12, "buy_volume"sv, instrument.trade_update.fills.buy_volume);
          print_helper(12, "sell_volume"sv, instrument.trade_update.fills.sell_volume);
          print_helper(12, "total_volume"sv, instrument.trade_update.fills.total_volume);
          print_helper(8, "position_update"sv);
          print_helper(10, "total_count"sv, instrument.position_update.total_count);
          print_helper(10, "position_min"sv, instrument.position_update.position_min);
          print_helper(10, "position_max"sv, instrument.position_update.position_max);
          print_helper(8, "history"sv);
          for (auto &[sample_period_utc, history] : instrument.history) {
            print_helper(10, "sample_period_utc"sv, sample_period_utc);
            print_helper(12, "best_bid_price"sv, history.best_bid_price);
            print_helper(12, "best_ask_price"sv, history.best_ask_price);
            print_helper(12, "buy_volume"sv, history.buy_volume);
            print_helper(12, "sell_volume"sv, history.sell_volume);
            print_helper(12, "position"sv, history.position);
            print_helper(12, "average_price"sv, history.average_price);
            print_helper(12, "mark_price"sv, history.mark_price);
            print_helper(12, "unrealized_profit"sv, history.unrealized_profit);
            print_helper(12, "realized_profit"sv, history.realized_profit);
          }
        }
      }
    }
  }

  void print_json(std::string_view const &label) const {}

  void print_csv_history() const {
    fmt::print("source,exchange,symbol,datetime_utc"sv);
    fmt::print(",best_bid_price,best_ask_price"sv);
    fmt::print(",buy_volume,sell_volume"sv);
    fmt::print(",position,realized_profit,unrealized_profit,average_price,mark_price"sv);
    fmt::print("\n"sv);
    for (size_t source = 0; source < std::size(instruments_); ++source) {
      auto &tmp_1 = instruments_[source];
      for (auto &[exchange, tmp_2] : tmp_1) {
        for (auto &[symbol, instrument] : tmp_2) {
          for (auto &[sample_period_utc, history] : instrument.history) {
            auto datetime_utc = std::chrono::duration_cast<std::chrono::seconds>(sample_period_utc);
            fmt::print(R"({},"{}","{}",{})"sv, source, exchange, symbol, datetime_utc.count());
            fmt::print(",{},{}"sv, history.best_bid_price, history.best_ask_price);
            fmt::print(",{},{}"sv, history.buy_volume, history.sell_volume);
            fmt::print(",{},{},{},{},{}"sv, history.position, history.realized_profit, history.unrealized_profit, history.average_price, history.mark_price);
            fmt::print("\n"sv);
          }
        }
      }
    }
  }

  void write(std::string_view const &, OutputType, std::string_view const &) const override {}

  // collector

  void operator()(Event<Timer> const &event) override { check(event); }

  void operator()(Event<Connected> const &event) override { check(event); }

  void operator()(Event<Disconnected> const &event) override { check(event); }

  void operator()(Event<Ready> const &event) override { check(event); }

  void operator()(Event<ReferenceData> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.reference_data.total_count;
      instrument.market_data(event);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketStatus> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.market_status.total_count;
      instrument.market_data(event);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<TopOfBook> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.top_of_book.total_count;
      if (instrument.market_data(event))
        update_history(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketByPriceUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.market_by_price_update.total_count;
      if (instrument.market_data(event))
        update_history(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketByOrderUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.market_by_order_update.total_count;
      if (instrument.market_data(event))
        update_history(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<TradeSummary> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.trade_summary.total_count;
      instrument.market_data(event);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<StatisticsUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.statistics_update.total_count;
      instrument.market_data(event);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<OrderAck> const &event) override {
    check(event);
    auto &[message_info, order_ack] = event;
    auto callback = [&](auto &instrument) {
      if (order_ack.origin != Origin::EXCHANGE)
        return;
      if (utils::has_request_completed(order_ack.request_status))
        ++instrument.order_ack.accepted_count;
      if (utils::has_request_failed(order_ack.request_status))
        ++instrument.order_ack.rejected_count;
    };
    get_instrument(event, callback);
  }

  void operator()(Event<OrderUpdate> const &event) override {
    check(event);
    auto &[message_info, order_update] = event;
    auto callback = [&](auto &instrument) {
      instrument(event);
      switch (order_update.side) {
        using enum Side;
        case UNDEFINED:
          assert(false);
          break;
        case BUY:
          ++instrument.order_update.buy_count;
          break;
        case SELL:
          ++instrument.order_update.sell_count;
          break;
      }
      ++instrument.order_update.total_count;
    };
    get_instrument(event, callback);
  }

  void operator()(Event<TradeUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      if (instrument(event))
        update_history(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<PositionUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      if (instrument(event))
        update_history(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<CustomMetricsUpdate> const &event) override {
    auto &[message_info, custom_metrics_update] = event;
    if (message_info.receive_time.count()) {  // XXX FIXME TODO doesn't yet work with the simulator (because of timing or source being SELF)
      // check(event);
    }
    // XXX TODO capture
  }

  void operator()(Event<CustomMatrixUpdate> const &event) override {
    auto &[message_info, custom_matrix_update] = event;
    if (message_info.receive_time.count()) {  // XXX FIXME TODO doesn't yet work with the simulator (because of timing or source being SELF)
      // check(event);
    }
    // XXX TODO capture
  }

  // utils

  template <typename T, typename Callback>
  void get_instrument(Event<T> const &event, Callback callback) {
    auto &[message_info, value] = event;
    instruments_.resize(std::max<size_t>(message_info.source + 1, std::size(instruments_)));
    auto &tmp = instruments_[message_info.source][value.exchange];
    auto iter = tmp.find(value.symbol);
    if (iter == std::end(tmp)) [[unlikely]] {
      auto res = tmp.try_emplace(value.symbol, value.exchange, value.symbol, market_data_source_);
      assert(res.second);
      iter = res.first;
    }
    auto &instrument = (*iter).second;
    callback(instrument);
  }

  void update_history(Instrument &instrument) {
    auto position = instrument.position_tracker.position();
    assert(!std::isnan(position));
    auto &top_of_book = instrument.market_data.top_of_book();  // XXX FIXME TODO use impact price using std::fabs(position) !!!
    auto mark_price = [&]() -> double {
      if (utils::compare(position, 0.0) == 0)
        return NaN;
      if (position > 0.0)
        return top_of_book.bid_price;
      return top_of_book.ask_price;
    }();
    auto multiplier = 1.0;  // XXX FIXME TODO get from reference data
    auto [realized_profit, unrealized_profit, average_price] = instrument.position_tracker.compute_pnl(mark_price, multiplier);
    auto [buy_volume, sell_volume, total_volume] = instrument.position_tracker.current_volume();
    assert(sample_period_utc_.count());
    if (std::empty(instrument.history) || instrument.history[std::size(instrument.history) - 1].first != sample_period_utc_) {
      auto sample = Instrument::Sample{
          .best_bid_price = top_of_book.bid_price,
          .best_ask_price = top_of_book.ask_price,
          .buy_volume = buy_volume,
          .sell_volume = sell_volume,
          .position = position,
          .average_price = average_price,
          .mark_price = mark_price,
          .unrealized_profit = unrealized_profit,
          .realized_profit = realized_profit,
      };
      instrument.history.emplace_back(sample_period_utc_, std::move(sample));
    } else {
      auto &sample = instrument.history[std::size(instrument.history) - 1].second;
      new (&sample) Instrument::Sample{
          .best_bid_price = top_of_book.bid_price,
          .best_ask_price = top_of_book.ask_price,
          .buy_volume = buy_volume,
          .sell_volume = sell_volume,
          .position = position,
          .average_price = average_price,
          .mark_price = mark_price,
          .unrealized_profit = unrealized_profit,
          .realized_profit = realized_profit,
      };
    }
  }

  static void print_helper(size_t indent, std::string_view const &label) { fmt::print("{: >{}}{}\n"sv, ""sv, indent, label); }
  static void print_helper(size_t indent, std::string_view const &label, auto const &value) { fmt::print("{: >{}}{}: {}\n"sv, ""sv, indent, label, value); }

  template <typename T>
  void check(Event<T> const &event) {
    auto &[message_info, value] = event;
    log::debug(
        "[{}:{}] receive_time={}, receive_time_utc={}, {}={}"sv,
        message_info.source,
        message_info.source_name,
        message_info.receive_time,
        message_info.receive_time_utc,
        get_name<T>(),
        value);
    time_checker_(event);
    // sample period
    auto sample_period_utc = (message_info.receive_time_utc / sample_frequency_) * sample_frequency_;
    if (sample_period_utc_ < sample_period_utc)
      sample_period_utc_ = sample_period_utc;
  }

 private:
  MarketDataSource const market_data_source_;
  std::chrono::nanoseconds const sample_frequency_;
  std::vector<utils::unordered_map<std::string, utils::unordered_map<std::string, Instrument>>> instruments_;
  std::chrono::nanoseconds sample_period_utc_ = {};
  // DEBUG
  tools::TimeChecker time_checker_;
};
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<Reporter> Summary::create() {
  return create(DEFAULT_CONFIG);
}

std::unique_ptr<Reporter> Summary::create(Config const &config) {
  return std::make_unique<Implementation>(config);
}

}  // namespace reporter
}  // namespace algo
}  // namespace roq
