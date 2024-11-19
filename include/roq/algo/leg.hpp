/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <string_view>

#include <fmt/core.h>

#include "roq/api.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Leg final {
  uint8_t source = {};
  Account account;
  Exchange exchange;
  Symbol symbol;
  PositionEffect position_effect = {};
  MarginMode margin_mode = {};
  TimeInForce time_in_force = {};
};

}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::Leg> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::Leg const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(source={}, )"
        R"(account="{}", )"
        R"(exchange="{}", )"
        R"(symbol="{}", )"
        R"(position_effect={}, )"
        R"(margin_mode={}, )"
        R"(time_in_force={})"
        R"(}})"sv,
        value.source,
        value.account,
        value.exchange,
        value.symbol,
        value.position_effect,
        value.margin_mode,
        value.time_in_force);
  }
};
