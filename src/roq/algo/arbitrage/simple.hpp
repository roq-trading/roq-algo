/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <limits>
#include <vector>

#include "roq/cache/market_by_order.hpp"
#include "roq/cache/market_by_price.hpp"
#include "roq/cache/market_status.hpp"
#include "roq/cache/top_of_book.hpp"

#include "roq/algo/cache.hpp"

#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"

#include "roq/algo/arbitrage/config.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct Simple final : public strategy::Handler {
  Simple(strategy::Dispatcher &, Cache &, std::string_view const &exchange, std::string_view const &symbol, Config const &);

  Simple(Simple const &) = delete;

 protected:
  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;

  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<MarketByOrderUpdate> const &) override;
  void operator()(Event<TradeSummary> const &) override;
  void operator()(Event<StatisticsUpdate> const &) override;

 private:
  strategy::Dispatcher &dispatcher_;
  Cache &cache_;
  Config const config_;
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
