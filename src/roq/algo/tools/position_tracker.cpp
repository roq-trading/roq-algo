/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/tools/position_tracker.hpp"

#include <cassert>

#include "roq/logging.hpp"

#include "roq/utils/compare.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace tools {

// === IMPLEMENTATION ===

std::tuple<double, double, double> PositionTracker::compute_pnl(double mark_price, [[maybe_unused]] double multiplier) const {
  auto tmp_1 = (mark_price * position_) - cost_;
  auto unrealized_profit = std::isnan(tmp_1) ? 0.0 : tmp_1;
  auto tmp_2 = cost_ / position_;
  auto average_price = std::isfinite(tmp_2) ? tmp_2 : NaN;
  return {
      realized_profit_,
      unrealized_profit,
      average_price,
  };
}

// XXX FIXME TODO
// NOLINTBEGIN(readability-function-cognitive-complexity)
void PositionTracker::operator()(Event<TradeUpdate> const &event) {
  auto &[message_info, trade_update] = event;
  switch (trade_update.update_type) {
    using enum UpdateType;
    case UNDEFINED:
      assert(false);  // note! should never happen
      break;
    case SNAPSHOT:
      // assert(false);  // XXX FIXME TODO support download
      break;
    case INCREMENTAL: {
      // note! switch is inside loop because price can be different for each fill + we need to track when position crosses long/short
      for (auto &item : trade_update.fills) {
        switch (trade_update.side) {
          using enum Side;
          case UNDEFINED:
            assert(false);
            break;
          case BUY: {
            auto position = position_ + item.quantity;
            if (utils::compare(position_, 0.0) < 0 && utils::compare(position, 0.0) >= 0) {  // note! close short
              cost_ -= position_ * item.price;
              realized_profit_ -= cost_;
              cost_ = position * item.price;
            } else {
              cost_ += item.quantity * item.price;
            }
            position_ = position;
            buy_volume_ += item.quantity;
            break;
          }
          case SELL: {
            auto position = position_ - item.quantity;
            if (utils::compare(position_, 0.0) > 0 && utils::compare(position, 0.0) <= 0) {  // note! close long
              cost_ -= position_ * item.price;
              realized_profit_ -= cost_;
              cost_ = position * item.price;
            } else {
              cost_ -= item.quantity * item.price;
            }
            position_ = position;
            sell_volume_ += item.quantity;
            break;
          }
        }
        total_volume_ += item.quantity;
      }
      break;
    }
    case STALE:
      assert(false);  // note! should never happen
      break;
  }
}
// NOLINTEND(readability-function-cognitive-complexity)

// XXX FIXME TODO simulator doesn't emit snapshot ???
void PositionTracker::operator()(Event<PositionUpdate> const &event) {
  auto &[message_info, position_update] = event;
  // assert(position_update.update_type != UpdateType::SNAPSHOT);
  current_position_ = position_update.long_quantity - position_update.short_quantity;
}

}  // namespace tools
}  // namespace algo
}  // namespace roq
