/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/utils/container.hpp"

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

struct Simple final : public strategy::Handler {
  using Dispatcher = strategy::Dispatcher;

  Simple(Dispatcher &, Config const &, Cache &);

  Simple(Simple &&) = delete;
  Simple(Simple const &) = delete;

  struct State final {
    bool ready = {};
    double tick_size = NaN;
    uint16_t stream_id = {};                          // market data stream
    std::chrono::nanoseconds latency = {};            // latest market data latency
    std::chrono::nanoseconds exchange_time_utc = {};  // latest market data update
    TradingStatus trading_status = {};
    Layer best;
  };

  struct Instrument final {
    uint8_t const source = {};
    std::string const exchange;
    std::string const symbol;
    // private:
    State state;
  };

 protected:
  void operator()(Event<Timer> const &) override;

  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;

  void operator()(Event<Ready> const &) override;

  void operator()(Event<StreamStatus> const &) override;
  void operator()(Event<ExternalLatency> const &) override;

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
  void get_all_states(MessageInfo const &, Callback);

  template <typename T, typename Callback>
  bool get_state(Event<T> const &, Callback);

  bool can_trade() const;

  void update();

  struct Source final {
    utils::unordered_map<std::string_view, utils::unordered_map<std::string_view, size_t>> const lookup;
    std::vector<std::chrono::nanoseconds> stream_latency;
  };

 private:
  Dispatcher &dispatcher_;
  // config
  std::chrono::nanoseconds const max_age_;  // used when trading status is unavailable
  SupportType const market_data_type_;
  // cache
  Cache &cache_;
  // state
  std::vector<Instrument> instruments_;
  std::vector<Source> sources_;
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
        R"(ready={}, )"
        R"(tick_size={}, )"
        R"(stream_id={}, )"
        R"(latency={}, )"
        R"(exchange_time_utc={}, )"
        R"(trading_status={}, )"
        R"(best={})"
        R"(}})"sv,
        value.ready,
        value.tick_size,
        value.stream_id,
        value.latency,
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
        R"(state={})"
        R"(}})"sv,
        value.source,
        value.exchange,
        value.symbol,
        value.state);
  }
};
