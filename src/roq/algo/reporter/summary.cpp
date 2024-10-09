/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/reporter/summary.hpp"

#include <cassert>
#include <vector>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

#include "roq/utils/container.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace reporter {

// === CONSTANTS ===

namespace {
auto const DEFAULT_CONFIG = Summary::Config{
    .frequency = 1min,
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
  explicit Implementation(Summary::Config const &config) : frequency_{config.frequency} {}

 protected:
  struct Instrument final {
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
      log::info("source: {}"sv, source);
      auto &tmp_1 = instruments_[source];
      for (auto &[exchange, tmp_2] : tmp_1) {
        log::info("  exchange: {}"sv, exchange);
        for (auto &[symbol, instrument] : tmp_2) {
          log::info("{: >{}}symbol: {}"sv, ""sv, 4, symbol);
          log::info("{: >{}}market_data"sv, ""sv, 6);
          log::info("{: >{}}reference_data"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.reference_data.total_count);
          log::info("{: >{}}market_status"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.market_status.total_count);
          log::info("{: >{}}top_of_book"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.top_of_book.total_count);
          log::info("{: >{}}market_by_price_update"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.market_by_price_update.total_count);
          log::info("{: >{}}market_by_order_update"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.market_by_order_update.total_count);
          log::info("{: >{}}trade_summary"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.trade_summary.total_count);
          log::info("{: >{}}statistics_update"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.statistics_update.total_count);
          log::info("{: >{}}order_management"sv, ""sv, 6);
          log::info("{: >{}}order_ack"sv, ""sv, 8);
          log::info("{: >{}}accepted_count: {}"sv, ""sv, 10, instrument.order_ack.accepted_count);
          log::info("{: >{}}rejected_count: {}"sv, ""sv, 10, instrument.order_ack.rejected_count);
          log::info("{: >{}}order_update"sv, ""sv, 8);
          log::info("{: >{}}buy_count: {}"sv, ""sv, 10, instrument.order_update.buy_count);
          log::info("{: >{}}sell_count: {}"sv, ""sv, 10, instrument.order_update.sell_count);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.order_update.total_count);
          log::info("{: >{}}trade_update"sv, ""sv, 8);
          log::info("{: >{}}fills"sv, ""sv, 10);
          log::info("{: >{}}buy_count: {}"sv, ""sv, 12, instrument.trade_update.fills.buy_count);
          log::info("{: >{}}sell_count: {}"sv, ""sv, 12, instrument.trade_update.fills.sell_count);
          log::info("{: >{}}total_count: {}"sv, ""sv, 12, instrument.trade_update.fills.total_count);
          log::info("{: >{}}buy_volume: {}"sv, ""sv, 12, instrument.trade_update.fills.buy_volume);
          log::info("{: >{}}sell_volume: {}"sv, ""sv, 12, instrument.trade_update.fills.sell_volume);
          log::info("{: >{}}total_volume: {}"sv, ""sv, 12, instrument.trade_update.fills.total_volume);
          log::info("{: >{}}position_update"sv, ""sv, 8);
          log::info("{: >{}}total_count: {}"sv, ""sv, 10, instrument.position_update.total_count);
          log::info("{: >{}}position_min: {}"sv, ""sv, 10, instrument.position_update.position_min);
          log::info("{: >{}}position_max: {}"sv, ""sv, 10, instrument.position_update.position_max);
          log::info("{: >{}}history"sv, ""sv, 8);
          for (auto &[sample_period_utc, history] : instrument.history) {
            log::info("{: >{}}sample_period_utc: {}"sv, ""sv, 10, sample_period_utc);
            log::info("{: >{}}price: {}"sv, ""sv, 12, history.price);
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
    // auto &[message_info, reference_data] = event;
    auto callback = [&](auto &instrument) { ++instrument.reference_data.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketStatus> const &event) override {
    check(event);
    // auto &[message_info, market_status] = event;
    auto callback = [&](auto &instrument) { ++instrument.market_status.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<TopOfBook> const &event) override {
    check(event);
    auto &[message_info, top_of_book] = event;
    auto callback = [&](auto &instrument) {
      ++instrument.top_of_book.total_count;
      // XXX FIXME TODO generalize this so we can use any market data source
      auto mid_price = 0.5 * (top_of_book.layer.bid_price + top_of_book.layer.ask_price);  // XXX FIXME TODO deal with one-sided and missing
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
    };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketByPriceUpdate> const &event) override {
    check(event);
    // auto &[message_info, market_by_price_update] = event;
    auto callback = [&](auto &instrument) { ++instrument.market_by_price_update.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<MarketByOrderUpdate> const &event) override {
    check(event);
    // auto &[message_info, market_by_order_update] = event;
    auto callback = [&](auto &instrument) { ++instrument.market_by_order_update.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<TradeSummary> const &event) override {
    check(event);
    // auto &[message_info, trade_summary] = event;
    auto callback = [&](auto &instrument) { ++instrument.trade_summary.total_count; };
    get_instrument(event, callback);
  }

  void operator()(Event<StatisticsUpdate> const &event) override {
    check(event);
    // auto &[message_info, statistics_update] = event;
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
    auto &instrument = instruments_[message_info.source][value.exchange][value.symbol];
    callback(instrument);
  }

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
      auto sample_period_utc = (message_info.receive_time_utc / frequency_) * frequency_;
      // note! the realtime isn't guaranteed to be monotonic and we must therefore protect against time-inversion
      if (sample_period_utc_ < sample_period_utc) {
        sample_period_utc_ = sample_period_utc;
        log::warn("HERE sample_period_utc={}"sv, sample_period_utc_);
      }
    }
  }

 private:
  std::chrono::nanoseconds const frequency_;
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
