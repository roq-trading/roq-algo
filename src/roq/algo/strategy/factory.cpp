/* Copyright (c) 2017-2026, Hans Erik Thrane */

#include "roq/algo/strategy/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/arbitrage/factory.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace strategy {

// === IMPLEMENTATION ===

std::unique_ptr<Strategy> Factory::create(
    Type type, Strategy::Dispatcher &dispatcher, OrderCache &order_cache, Config const &config, std::string_view const &parameters) {
  switch (type) {
    using enum Type;
    case UNDEFINED:
      break;
    case ARBITRAGE:
      return arbitrage::Factory::create(dispatcher, order_cache, config, parameters);
  }
  log::fatal("Unexpected"sv);
}

std::span<strategy::Meta const> Factory::get_meta(Type type) {
  switch (type) {
    using enum Type;
    case UNDEFINED:
      break;
    case ARBITRAGE:
      return arbitrage::Factory::get_meta();
  }
  log::fatal("Unexpected"sv);
}

}  // namespace strategy
}  // namespace algo
}  // namespace roq
