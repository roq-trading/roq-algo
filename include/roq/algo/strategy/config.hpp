/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <vector>

#include "roq/algo/leg.hpp"

namespace roq {
namespace algo {
namespace strategy {

struct ROQ_PUBLIC Config final {
  static Config parse_file(std::string_view const &path);
  static Config parse_text(std::string_view const &text);

  std::vector<Leg> legs;
  uint32_t strategy_id = {};
};

}  // namespace strategy
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::strategy::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::strategy::Config const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(legs=[{}], )"
        R"(strategy_id={})"
        R"(}})"sv,
        fmt::join(value.legs, ", "sv),
        value.strategy_id);
  }
};
