/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/tools/position_tracker.hpp"

#include <cassert>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace tools {

// === IMPLEMENTATION ===

std::pair<double, double> PositionTracker::compute_pnl(double current_price, double multiplier) const {
  auto profit = current_price * position_ - cost_;  // note! simplified calculation of p&l
  return {position_, profit};
}

void PositionTracker::operator()(Event<TradeUpdate> const &event) {
  auto &[message_info, trade_update] = event;
  switch (trade_update.update_type) {
    using enum UpdateType;
    case UNDEFINED:
      assert(false);  // note! should never happen
      break;
    case SNAPSHOT:
      assert(false);  // XXX FIXME TODO support download
      break;
    case INCREMENTAL: {
      auto sign = utils::sign(trade_update.side);
      for (auto &item : trade_update.fills) {
        auto amount = item.quantity * sign;
        position_ += amount;
        cost_ += amount * item.price;
      }
      break;
    }
    case STALE:
      assert(false);  // note! should never happen
      break;
  }
}

// XXX FIXME TODO simulator doesn't emit snapshot ???
void PositionTracker::operator()(Event<PositionUpdate> const &event) {
  auto &[message_info, position_update] = event;
  assert(position_update.update_type != UpdateType::SNAPSHOT);
  current_position_ = position_update.long_quantity - position_update.short_quantity;
}

}  // namespace tools
}  // namespace algo
}  // namespace roq
