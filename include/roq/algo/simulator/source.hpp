/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <chrono>
#include <climits>

#include "roq/limits.hpp"
#include "roq/string_types.hpp"

#include "roq/utils/container.hpp"

namespace roq {
namespace algo {
namespace simulator {

struct ROQ_PUBLIC Position final {
  double long_position = NaN;
  double short_position = NaN;
};

struct ROQ_PUBLIC Symbol final {
  Position position;
};

struct ROQ_PUBLIC Exchange final {
  utils::unordered_map<std::string, Symbol> symbols;
};

struct ROQ_PUBLIC Account final {
  utils::unordered_map<std::string, Exchange> exchanges;
};

struct ROQ_PUBLIC Source final {
  std::chrono::milliseconds market_data_latency = {};
  std::chrono::milliseconds order_management_latency = {};
  utils::unordered_map<std::string, Account> accounts;
};

}  // namespace simulator
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::simulator::Position> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::simulator::Position const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(long_position={}, )"
        R"(short_position={})"
        R"(}})"sv,
        value.long_position,
        value.short_position);
  }
};

template <>
struct fmt::formatter<roq::algo::simulator::Symbol> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::simulator::Symbol const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(position={})"
        R"(}})"sv,
        value.position);
  }
};

template <>
struct fmt::formatter<roq::algo::simulator::Exchange> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::simulator::Exchange const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(symbols={})"
        R"(}})"sv,
        fmt::join(value.symbols, ", "sv));
  }
};

template <>
struct fmt::formatter<roq::algo::simulator::Account> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::simulator::Account const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(exchanges={})"
        R"(}})"sv,
        fmt::join(value.exchanges, ", "sv));
  }
};

template <>
struct fmt::formatter<roq::algo::simulator::Source> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::simulator::Source const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(market_data_latency={}, )"
        R"(order_management_latency={}, )"
        R"(accounts={})"
        R"(}})"sv,
        value.market_data_latency,
        value.order_management_latency,
        fmt::join(value.accounts, ", "sv));
  }
};
