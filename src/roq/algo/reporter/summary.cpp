/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/reporter/summary.hpp"

#include <cassert>
#include <vector>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/container.hpp"

#include "roq/algo/tools/market.hpp"

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
// TODO
// -- first/last event receive time (utc)
// -- fix min position
// -- timer + event => bars (need frequency)
// ---- price (close), profit, #orders, vol#orders, #fills, vol#fills
// -- custom metrics

struct Implementation final : public Handler {
  explicit Implementation(Summary::Config const &config) : market_data_source_{config.market_data_source}, sample_frequency_{config.sample_frequency} {}

 protected:
  struct Instrument final {
    Instrument(std::string_view const &exchange, std::string_view const &symbol, MarketDataSource market_data_source)
        : market{exchange, symbol, market_data_source} {}

    tools::Market market;

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
    // history
    struct History final {
      double price = NaN;  // last
    };
    std::vector<std::pair<std::chrono::nanoseconds, History>> history;
  };

  // reporter

  void print() const override {
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
            print_helper(12, "price"sv, history.price);
          }
        }
      }
    }
  }

  // collector

  void operator()(Event<Timer> const &event) override { check(event); }

  void operator()(Event<Connected> const &event) override { check(event); }

  void operator()(Event<Disconnected> const &event) override { check(event); }

  void operator()(Event<Ready> const &event) override { check(event); }

  void operator()(Event<ReferenceData> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.reference_data.total_count;
      instrument.market(event);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketStatus> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.market_status.total_count;
      instrument.market(event);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<TopOfBook> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.top_of_book.total_count;
      if (instrument.market(event))
        update_best(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketByPriceUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.market_by_price_update.total_count;
      if (instrument.market(event))
        update_best(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketByOrderUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) {
      ++instrument.market_by_order_update.total_count;
      if (instrument.market(event))
        update_best(instrument);
    };
    get_instrument(event, callback);
  }

  void operator()(Event<TradeSummary> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) { ++instrument.trade_summary.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<StatisticsUpdate> const &event) override {
    check(event);
    auto callback = [&](auto &instrument) { ++instrument.statistics_update.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<OrderAck> const &event) override {
    check(event);
    auto &[message_info, order_ack] = event;
    auto callback = [&](auto &instrument) {
      if (order_ack.origin != Origin::EXCHANGE)
        return;
      if (roq::utils::has_request_completed(order_ack.request_status))
        ++instrument.order_ack.accepted_count;
      if (roq::utils::has_request_failed(order_ack.request_status))
        ++instrument.order_ack.rejected_count;
    };
    get_instrument(event, callback);
  }

  void operator()(Event<OrderUpdate> const &event) override {
    check(event);
    auto &[message_info, order_update] = event;
    auto callback = [&](auto &instrument) {
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
    auto &[message_info, trade_update] = event;
    auto callback = [&](auto &instrument) {
      for (auto &fill : trade_update.fills) {
        assert(roq::utils::compare(fill.quantity, 0.0) > 0);
        switch (trade_update.side) {
          using enum Side;
          case UNDEFINED:
            assert(false);
            break;
          case BUY:
            ++instrument.trade_update.fills.buy_count;
            instrument.trade_update.fills.buy_volume += fill.quantity;
            break;
          case SELL:
            ++instrument.trade_update.fills.sell_count;
            instrument.trade_update.fills.sell_volume += fill.quantity;
            break;
        }
        ++instrument.trade_update.fills.total_count;
        instrument.trade_update.fills.total_volume += fill.quantity;
      }
    };
    get_instrument(event, callback);
  }

  void operator()(Event<PositionUpdate> const &event) override {
    check(event);
    auto &[message_info, position_update] = event;
    auto callback = [&](auto &instrument) {
      ++instrument.position_update.total_count;
      auto position = position_update.long_quantity - position_update.short_quantity;
      roq::utils::update_min(instrument.position_update.position_min, position);
      roq::utils::update_max(instrument.position_update.position_max, position);
    };
    get_instrument(event, callback);
  }

  // utils

  template <typename T, typename Callback>
  void get_instrument(Event<T> const &event, Callback callback) {
    auto &[message_info, value] = event;
    instruments_.resize(std::max<size_t>(message_info.source + 1, std::size(instruments_)));
    auto &tmp = instruments_[message_info.source][value.exchange];
    auto iter = tmp.find(value.symbol);
    if (iter == std::end(tmp)) [[unlikely]] {
      auto res = tmp.try_emplace(value.symbol, value.exchange, value.symbol, MarketDataSource::TOP_OF_BOOK);
      assert(res.second);
      iter = res.first;
    }
    auto &instrument = (*iter).second;
    callback(instrument);
  }

  void update_best(Instrument &instrument) {
    auto &best = instrument.market.get_best();
    auto mid_price = 0.5 * (best.bid_price + best.ask_price);  // XXX FIXME TODO deal with one-sided and missing
    assert(sample_period_utc_.count());
    if (std::empty(instrument.history) || instrument.history[std::size(instrument.history) - 1].first != sample_period_utc_) {
      auto history = Instrument::History{
          .price = mid_price,
      };
      instrument.history.emplace_back(sample_period_utc_, std::move(history));
    } else {
      auto &history = instrument.history[std::size(instrument.history) - 1].second;
      history.price = mid_price;
    }
  }

  static void print_helper(size_t indent, std::string_view const &label) { fmt::print("{: >{}}{}\n"sv, ""sv, indent, label); }
  static void print_helper(size_t indent, std::string_view const &label, auto const &value) { fmt::print("{: >{}}{}: {}\n"sv, ""sv, indent, label, value); }

  template <typename T>
  void check(Event<T> const &event) {
    using value_type = std::remove_cvref<T>::type;
    auto &[message_info, value] = event;
    auto helper = [](auto &lhs, auto rhs) {
      std::chrono::nanoseconds result;
      if (lhs.count()) {
        result = rhs - lhs;
      } else {
        result = {};
      }
      lhs = rhs;
      return result;
    };
    auto diff = helper(last_receive_time_, message_info.receive_time);
    auto diff_utc = helper(last_receive_time_utc_, message_info.receive_time_utc);  // XXX FIXME TODO track by source
    log::debug(
        "[{}:{}] receive_time={}({}), receive_time_utc={}({}), {}={}"sv,
        message_info.source,
        message_info.source_name,
        message_info.receive_time,
        diff,
        message_info.receive_time_utc,
        diff_utc,
        get_name<T>(),
        value);
    assert(!std::empty(message_info.source_name) || message_info.source == SOURCE_SELF);
    assert(message_info.receive_time.count());
    assert(diff >= 0ns);
    if constexpr (std::is_same<value_type, Connected>::value) {
      // XXX FIXME simulator doesn't populate receive_time_utc
    } else {
      assert(message_info.receive_time_utc.count());
      // note! diff_utc can be negative (clock adjustment, sampling from different cores, etc.)
      // ...
      auto sample_period_utc = (message_info.receive_time_utc / sample_frequency_) * sample_frequency_;
      // note! the realtime isn't guaranteed to be monotonic and we must therefore protect against time-inversion
      if (sample_period_utc_ < sample_period_utc) {
        sample_period_utc_ = sample_period_utc;
        log::warn("HERE sample_period_utc={}"sv, sample_period_utc_);
      }
    }
  }

 private:
  MarketDataSource const market_data_source_;
  std::chrono::nanoseconds const sample_frequency_;
  std::vector<utils::unordered_map<std::string, utils::unordered_map<std::string, Instrument>>> instruments_;
  std::chrono::nanoseconds last_receive_time_ = {};
  std::chrono::nanoseconds last_receive_time_utc_ = {};
  std::chrono::nanoseconds sample_period_utc_ = {};
};
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<Handler> Summary::create() {
  return create(DEFAULT_CONFIG);
}

std::unique_ptr<Handler> Summary::create(Config const &config) {
  return std::make_unique<Implementation>(config);
}

}  // namespace reporter
}  // namespace algo
}  // namespace roq