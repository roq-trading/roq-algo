/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/matcher/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/matcher/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace matcher {

// === IMPLEMENTATION ===

std::unique_ptr<Matcher> Factory::create(Type type, Matcher::Dispatcher &dispatcher, std::string_view const &exchange, std::string_view const &symbol) {
  switch (type) {
    using enum Factory::Type;
    case SIMPLE:
      return std::make_unique<Simple>(dispatcher, exchange, symbol, Simple::MatchingSource::TOP_OF_BOOK);
  }
  log::fatal("Unexpected"sv);
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
