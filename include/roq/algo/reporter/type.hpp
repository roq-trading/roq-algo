/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <magic_enum.hpp>

#include <fmt/core.h>

namespace roq {
namespace algo {
namespace reporter {

enum class Type {
  NONE,
  SUMMARY,
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::reporter::Type> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Type const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}"sv, magic_enum::enum_name(value));
  }
};
