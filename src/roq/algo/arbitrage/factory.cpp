/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/factory.hpp"

#include "roq/algo/arbitrage/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === IMPLEMENTATION ===

std::unique_ptr<strategy::Handler> Factory::create(strategy::Dispatcher &dispatcher, Config const &config, OrderCache &order_cache) {
  return std::make_unique<Simple>(dispatcher, config, order_cache);
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
