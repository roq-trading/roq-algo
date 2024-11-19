/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/matcher/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/matcher/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace matcher {

// === IMPLEMENTATION ===

std::unique_ptr<Matcher> Factory::create(Type type, Matcher::Dispatcher &dispatcher, OrderCache &order_cache, Config const &config) {
  switch (type) {
    using enum Type;
    case SIMPLE:
      return std::make_unique<Simple>(dispatcher, order_cache, config);
  }
  log::fatal("Unexpected: type={}"sv, magic_enum::enum_name(type));
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
