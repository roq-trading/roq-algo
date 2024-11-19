/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <magic_enum/magic_enum.hpp>

#include <fmt/core.h>

namespace roq {
namespace algo {
namespace reporter {

enum class OutputType {
  TEXT,
  JSON,
  CSV,
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::reporter::OutputType> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::OutputType const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(context.out(), "{}"sv, magic_enum::enum_name(value));
  }
};
