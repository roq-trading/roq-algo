/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <chrono>

#include "roq/api.hpp"

namespace roq {
namespace algo {
namespace tools {

struct TimeChecker final {
  void operator()(MessageInfo const &message_info) { check(message_info); }

 protected:
  void check(MessageInfo const &);

 private:
  std::chrono::nanoseconds last_receive_time_ = {};
  std::chrono::nanoseconds last_receive_time_utc_ = {};
};

}  // namespace tools
}  // namespace algo
}  // namespace roq
