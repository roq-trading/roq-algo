/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Shared final {
  bool ready = {};
  uint64_t max_order_id = {};
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
