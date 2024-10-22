/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <magic_enum.hpp>

#include <fmt/core.h>

namespace roq {
namespace algo {
namespace strategy {

enum class Type {
  UNDEFINED,
  ARBITRAGE,
};

}  // namespace strategy
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::strategy::Type> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::strategy::Type const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}"sv, magic_enum::enum_name(value));
  }
};
