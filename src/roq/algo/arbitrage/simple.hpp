/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/algo/cache.hpp"

#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"

#include "roq/algo/arbitrage/config.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct Simple final : public strategy::Handler {
  using Dispatcher = strategy::Dispatcher;

  Simple(Dispatcher &, Config const &, Cache &);

  Simple(Simple const &) = delete;

 protected:
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;

  void operator()(Event<Ready> const &) override;

  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;

  void operator()(Event<TopOfBook> const &) override;

  void operator()(Event<OrderAck> const &, cache::Order const &) override;
  void operator()(Event<OrderUpdate> const &, cache::Order const &) override;
  void operator()(Event<TradeUpdate> const &, cache::Order const &) override;

  struct State final {
    bool ready = {};
    double tick_size = NaN;
    TradingStatus trading_status = {};
    Layer best;
  };

  State &get_state(MessageInfo const &);

 private:
  Dispatcher &dispatcher_;
  Config const config_;
  Cache &cache_;
  std::vector<State> state_;
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
