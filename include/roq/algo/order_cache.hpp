/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/cache/order.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC OrderCache {
  template <typename Callback>
  bool get_order(uint64_t order_id, Callback callback) {
    auto order = get_order_helper(order_id);
    if (order == nullptr) {
      return false;
    }
    callback(*order);
    return true;
  }

  virtual uint64_t get_next_trade_id() = 0;  // note! only used by matcher

 private:
  virtual cache::Order *get_order_helper(uint64_t order_id) = 0;
};

}  // namespace algo
}  // namespace roq
