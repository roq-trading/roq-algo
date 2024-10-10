/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <utility>

#include "roq/api.hpp"

namespace roq {
namespace algo {
namespace tools {

struct PositionTracker final {
  double current_position() const { return current_position_; }
  // note! returns {realized, unrealized}
  std::pair<double, double> compute_pnl(double current_price, double multiplier) const;

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
  double current_position_ = 0.0;
};

}  // namespace tools
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::tools::PositionTracker> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::tools::PositionTracker const &value, format_context &context) const { return value.format_helper(context.out()); }
};
