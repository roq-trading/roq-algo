/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/utils/common.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace utils {

// === IMPLEMENTATION ===

SupportType to_support_type(MarketDataSource market_data_source) {
  switch (market_data_source) {
    using enum MarketDataSource;
    case TOP_OF_BOOK:
      return SupportType::TOP_OF_BOOK;
    case MARKET_BY_PRICE:
      return SupportType::MARKET_BY_PRICE;
    case MARKET_BY_ORDER:
      return SupportType::MARKET_BY_ORDER;
  }
  log::fatal("Unexpected"sv);
}

}  // namespace utils
}  // namespace algo
}  // namespace roq
