/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/core.h>

#include "roq/algo/matcher/source.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Config final {
  Source source = {};
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
        R"(source={})"
        R"(}})"sv,
        value.source);
  }
};
