/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/utils/container.hpp"

#include "roq/cache/market_by_order.hpp"
#include "roq/cache/market_by_price.hpp"

#include "roq/algo/cache.hpp"
#include "roq/algo/market_data_source.hpp"

#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"

#include "roq/algo/arbitrage/config.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

// simple arbitrage
//
// prepared to support a list of instruments (n >= 2)
//
// assumptions:
// - only supporting positions (*not* FX-style)
//
// Q:
// - trading size
// - scaling factor?
// - max position
// - latency
// - liquid vs illiquid leg?
// - multiplier (compare real size)
// - threshold

struct Simple final : public strategy::Handler {
  using Dispatcher = strategy::Dispatcher;

  Simple(Dispatcher &, Config const &, Cache &);

  Simple(Simple &&) = delete;
  Simple(Simple const &) = delete;

  struct State final {
    double tick_size = NaN;
    double multiplier = NaN;
    double min_trade_vol = NaN;
    std::chrono::nanoseconds exchange_time_utc = {};  // latest market data update
    TradingStatus trading_status = {};
    Layer best;
    std::unique_ptr<cache::MarketByPrice> market_by_price;
    std::unique_ptr<cache::MarketByOrder> market_by_order;
    // DEBUG
    bool latch = {};
  };

  struct Instrument final {
    uint8_t const source = {};
    std::string const exchange;
    std::string const symbol;
    std::string const account;
    // private:
    State state;
  };

 protected:
  void operator()(Event<Timer> const &) override;

  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<DownloadEnd> const &) override;

  void operator()(Event<Ready> const &) override;

  void operator()(Event<StreamStatus> const &) override;
  void operator()(Event<ExternalLatency> const &) override;

  void operator()(Event<GatewayStatus> const &) override;

  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;

  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<MarketByOrderUpdate> const &) override;
  void operator()(Event<TradeSummary> const &) override;

  void operator()(Event<OrderAck> const &, cache::Order const &) override;
  void operator()(Event<OrderUpdate> const &, cache::Order const &) override;

  void operator()(Event<PositionUpdate> const &) override;

  // utils

  template <typename Callback>
  void get_instruments_by_source(MessageInfo const &, Callback);

  template <typename T, typename Callback>
  bool get_instrument(Event<T> const &, Callback);

  bool can_trade() const;

  void maybe_trade(Side, Instrument &lhs, Instrument &rhs);

  void update();

  struct Order final {
    Side side = {};
    double quantity = NaN;
    OrderStatus order_status = {};
  };

  struct Account final {
    bool has_download_orders = {};
    utils::unordered_map<size_t, Order> working_orders_by_instrument;  // <<< maybe in instrument
  };

  struct Source final {
    utils::unordered_map<std::string_view, Account> accounts;
    utils::unordered_map<std::string_view, utils::unordered_map<std::string_view, size_t>> const instruments;
    bool ready = {};
    std::vector<std::chrono::nanoseconds> stream_latency;
    utils::unordered_map<uint64_t, Order> working_orders;
  };

  template <typename T, typename Callback>
  bool get_account(Event<T> const &, Callback);

  template <typename T>
  void check(Event<T> const &);

 private:
  Dispatcher &dispatcher_;
  // config
  uint32_t const strategy_id_;
  std::chrono::nanoseconds const max_age_;  // used when trading status is unavailable
  SupportType const market_data_type_;
  double const threshold_;
  double const quantity_0_;
  double const min_position_0_;
  double const max_position_0_;
  // cache
  Cache &cache_;
  // state
  std::vector<Instrument> instruments_;
  std::vector<Source> sources_;
  uint64_t max_order_id_ = {};
  // DEBUG
  std::chrono::nanoseconds last_receive_time_ = {};
  std::chrono::nanoseconds last_receive_time_utc_ = {};
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::arbitrage::Simple::State> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::arbitrage::Simple::State const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(tick_size={}, )"
        R"(multiplier={}, )"
        R"(min_trade_vol={}, )"
        R"(exchange_time_utc={}, )"
        R"(trading_status={}, )"
        R"(best={})"
        R"(}})"sv,
        value.tick_size,
        value.multiplier,
        value.min_trade_vol,
        value.exchange_time_utc,
        value.trading_status,
        value.best);
  }
};

template <>
struct fmt::formatter<roq::algo::arbitrage::Simple::Instrument> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::arbitrage::Simple::Instrument const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(source={}, )"
        R"(exchange="{}", )"
        R"(symbol="{}", )"
        R"(account="{}", )"
        R"(state={})"
        R"(}})"sv,
        value.source,
        value.exchange,
        value.symbol,
        value.account,
        value.state);
  }
};
