/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <chrono>

#include "roq/message_info.hpp"

namespace roq {
namespace algo {
namespace tools {

struct ROQ_PUBLIC TimeChecker final {
  void operator()([[maybe_unused]] MessageInfo const &message_info) {
#ifndef NDEBUG
    check(message_info);
#endif
  }

 protected:
  void check(MessageInfo const &);

 private:
  std::chrono::nanoseconds last_receive_time_ = {};
  std::chrono::nanoseconds last_receive_time_utc_ = {};
};

}  // namespace tools
}  // namespace algo
}  // namespace roq
