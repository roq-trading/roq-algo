/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <chrono>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include "roq/api.hpp"

#include "roq/algo/market_data_source.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct ROQ_PUBLIC Parameters final {
  MarketDataSource market_data_source = {};
  std::chrono::nanoseconds max_age = {};  // used when exchange doesn't support trading status
  double threshold = NaN;                 // abs(spread) must exceed this threshold
  double quantity_0 = NaN;
  double min_position_0 = NaN;
  double max_position_0 = NaN;
  uint8_t publish_source = {};
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::arbitrage::Parameters> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::arbitrage::Parameters const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(market_data_source={}, )"
        R"(max_age={}, )"
        R"(threshold={}, )"
        R"(quantity_0={}, )"
        R"(min_position_0={}, )"
        R"(max_position_0={}, )"
        R"(publish_source={})"
        R"(}})"sv,
        value.market_data_source,
        value.max_age,
        value.threshold,
        value.quantity_0,
        value.min_position_0,
        value.max_position_0,
        value.publish_source);
  }
};
