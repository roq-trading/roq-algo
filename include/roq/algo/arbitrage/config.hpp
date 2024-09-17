/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <span>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include "roq/algo/instrument.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct ROQ_PUBLIC Config final {
  std::span<Instrument const> instruments;
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::arbitrage::Config> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::arbitrage::Config const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(instruments=[{}])"
        R"(}})"sv,
        fmt::join(value.instruments, ", "sv));
  }
};
