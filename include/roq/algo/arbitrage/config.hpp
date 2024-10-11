/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <chrono>
#include <span>

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include "roq/api.hpp"

#include "roq/algo/instrument.hpp"
#include "roq/algo/market_data_source.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct ROQ_PUBLIC Config final {
  uint32_t strategy_id = {};
  std::span<Instrument const> instruments;
  MarketDataSource market_data_source = {};
  std::chrono::nanoseconds max_age = {};  // when exchange doesn't support trading status
  double threshold = NaN;
  double quantity_0 = NaN;
  double min_position_0 = NaN;
  double max_position_0 = NaN;
  // XXX FIXME TODO following should be by instrument (leg)...
  PositionEffect position_effect = {};
  MarginMode margin_mode = {};
  TimeInForce time_in_force = {};
  //
  uint8_t publish_source = {};
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
        R"(strategy_id={}, )"
        R"(instruments=[{}], )"
        R"(market_data_source={}, )"
        R"(max_age={}, )"
        R"(threshold={}, )"
        R"(quantity_0={}, )"
        R"(min_position_0={}, )"
        R"(max_position_0={}, )"
        R"(position_effect={}, )"
        R"(margin_mode={}, )"
        R"(time_in_force={}, )"
        R"(publish_source={})"
        R"(}})"sv,
        value.strategy_id,
        fmt::join(value.instruments, ", "sv),
        value.market_data_source,
        value.max_age,
        value.threshold,
        value.quantity_0,
        value.min_position_0,
        value.max_position_0,
        value.position_effect,
        value.margin_mode,
        value.time_in_force,
        value.publish_source);
  }
};
