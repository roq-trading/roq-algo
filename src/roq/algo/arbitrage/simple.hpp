/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/utils/container.hpp"

#include "roq/algo/cache.hpp"

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

  void operator()(Event<OrderAck> const &, cache::Order const &) override;
  void operator()(Event<OrderUpdate> const &, cache::Order const &) override;

  void operator()(Event<PositionUpdate> const &) override;

  // utils

  struct State final {
    bool ready = {};
    double tick_size = NaN;
    TradingStatus trading_status = {};
    Layer best;
  };

  template <typename Callback>
  void get_state(MessageInfo const &, Callback callback);

  template <typename T>
  State &get_state(Event<T> const &);

 private:
  Dispatcher &dispatcher_;
  // config
  std::vector<utils::unordered_map<std::string, utils::unordered_map<std::string, size_t>>> const lookup_;
  // cache
  Cache &cache_;
  // state
  std::vector<State> state_;
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
