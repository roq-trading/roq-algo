/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/factory.hpp"

#include "roq/algo/arbitrage/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === IMPLEMENTATION ===

std::unique_ptr<strategy::Handler> Factory::create(
    strategy::Dispatcher &dispatcher, Cache &cache, std::string_view const &exchange, std::string_view const &symbol, Config const &config) {
  return std::make_unique<Simple>(dispatcher, cache, exchange, symbol, config);
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
