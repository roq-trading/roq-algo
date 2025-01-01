/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/core.h>

#include <magic_enum/magic_enum_format.hpp>

#include "roq/algo/market_data_source.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Config final {
  uint8_t source = {};
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
        R"(source={}, )"
        R"(exchange="{}", )"
        R"(symbol="{}", )"
        R"(market_data_source={})"
        R"(}})"sv,
        value.source,
        value.exchange,
        value.symbol,
        value.market_data_source);
  }
};
