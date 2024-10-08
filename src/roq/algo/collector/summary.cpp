/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/collector/summary.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace collector {

// === IMPLEMENTATION ===

void Summary::operator()(Event<TradeUpdate> const &event) {
  log::warn("event={}"sv, event);
}

}  // namespace collector
}  // namespace algo
}  // namespace roq
