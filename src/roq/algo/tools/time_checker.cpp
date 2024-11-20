/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/tools/time_checker.hpp"

#include <cassert>

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace tools {

// === IMPLEMENTATION ===

void TimeChecker::check(MessageInfo const &message_info) {
  auto helper = [](auto &lhs, auto rhs) {
    std::chrono::nanoseconds result;
    if (lhs.count()) {
      result = rhs - lhs;
    } else {
      result = {};
    }
    lhs = rhs;
    return result;
  };
  auto diff = helper(last_receive_time_, message_info.receive_time);
  [[maybe_unused]] auto diff_utc = helper(last_receive_time_utc_, message_info.receive_time_utc);  // XXX FIXME TODO track by source
  assert(!std::empty(message_info.source_name) || message_info.source == SOURCE_SELF);             // not really required, but this is a good place to check
  assert(message_info.receive_time.count());
  assert(diff >= 0ns);
  assert(message_info.receive_time_utc.count());
  // note! diff_utc can be negative (clock adjustment, sampling from different cores, etc.)
  if (diff < 0ns) [[unlikely]]
    log::fatal("Unexpected: internal error"sv);
}

}  // namespace tools
}  // namespace algo
}  // namespace roq
