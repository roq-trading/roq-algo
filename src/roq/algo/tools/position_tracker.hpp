/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <utility>

#include "roq/api.hpp"

namespace roq {
namespace algo {
namespace tools {

struct PositionTracker final {
  // note! PositionUpdate

  double current_position() const { return current_position_; }

  // note! from TradeUpdate

  double position() const { return position_; }

  // note! returns {realized_profit, unrealized_profit, average_cost_price}
  std::tuple<double, double, double> compute_pnl(double mark_price, double multiplier) const;

  // note! returns {buy_volume, sell_volume, total_volume}
  std::tuple<double, double, double> current_volume() const { return {buy_volume_, sell_volume_, total_volume_}; }

  void operator()(Event<TradeUpdate> const &);
  void operator()(Event<PositionUpdate> const &);

  template <typename OutputIt>
  auto constexpr format_helper(OutputIt out) const {
    using namespace std::literals;
    return fmt::format_to(
        out,
        R"({{)"
        R"(current_position={})"
        R"(}})"sv,
        current_position_);
  }

 private:
  double current_position_ = 0.0;  // note! from PositionUpdate
  // ...
  double position_ = 0.0;  // note! from TradeUpdate
  double cost_ = 0.0;
  double realized_profit_ = 0.0;
  // ...
  double buy_volume_ = 0.0;
  double sell_volume_ = 0.0;
  double total_volume_ = 0.0;
};

}  // namespace tools
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::tools::PositionTracker> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::tools::PositionTracker const &value, format_context &context) const { return value.format_helper(context.out()); }
};
