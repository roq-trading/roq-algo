/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/strategy.hpp"

#include "roq/algo/strategy/factory.hpp"

using namespace std::literals;

namespace roq {
namespace algo {

// === IMPLEMENTATION ===

std::unique_ptr<Strategy> Strategy::create(
    strategy::Type type, Dispatcher &dispatcher, OrderCache &order_cache, strategy::Config const &config, std::string_view const &parameters) {
  return strategy::Factory::create(type, dispatcher, order_cache, config, parameters);
}

}  // namespace algo
}  // namespace roq
