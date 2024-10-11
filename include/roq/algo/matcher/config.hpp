/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/core.h>

#include "roq/algo/market_data_source.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Config final {
  std::string_view exchange;
  std::string_view symbol;
  MarketDataSource market_data_source = {};
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::matcher::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::matcher::Config const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchange="{}", )"
        R"(symbol="{}", )"
        R"(market_data_source={})"
        R"(}})"sv,
        value.exchange,
        value.symbol,
        value.market_data_source);
  }
};
