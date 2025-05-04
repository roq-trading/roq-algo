/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <string_view>
#include <vector>

#include "roq/algo/simulator/source.hpp"

namespace roq {
namespace algo {
namespace simulator {

struct ROQ_PUBLIC Config final {
  static Config parse_file(std::string_view const &path);
  static Config parse_text(std::string_view const &text);

  std::vector<Source> sources;
};

}  // namespace simulator
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::simulator::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::simulator::Config const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(sources=[{}])"
        R"(}})"sv,
        fmt::join(value.sources, ", "sv));
  }
};
