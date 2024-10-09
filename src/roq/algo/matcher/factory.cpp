/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/matcher/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/matcher/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace matcher {

// === IMPLEMENTATION ===

std::unique_ptr<Handler> Factory::create(Type type, Dispatcher &dispatcher, Config const &config, OrderCache &order_cache) {
  switch (type) {
    using enum Factory::Type;
    case SIMPLE:
      return std::make_unique<Simple>(dispatcher, config, order_cache);
  }
  log::fatal("Unexpected: type={}"sv, magic_enum::enum_name(type));
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
