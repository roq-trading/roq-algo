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

  Simple(Simple const &) = delete;

  struct State final {
    bool ready = {};
    double tick_size = NaN;
    std::chrono::nanoseconds exchange_time_utc = {};  // latest market data update
    TradingStatus trading_status = {};
    Layer best;
  };

 protected:
  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;

  void operator()(Event<Ready> const &) override;

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

 private:
  Dispatcher &dispatcher_;
  // config
  std::vector<utils::unordered_map<std::string, utils::unordered_map<std::string, size_t>>> const lookup_;
  MarketDataSource const market_data_source_;
  std::chrono::nanoseconds const max_age_;
  // cache
  Cache &cache_;
  // state
  std::vector<State> state_;  // XXX FIXME rename... leg? instrument?
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
        R"(exchange_time_utc={}, )"
        R"(trading_status={}, )"
        R"(best={})"
        R"(}})"sv,
        value.ready,
        value.tick_size,
        value.exchange_time_utc,
        value.trading_status,
        value.best);
  }
};
