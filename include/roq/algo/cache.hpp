/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/cache/order.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Cache {
  template <typename Callback>
  bool get_order(uint64_t order_id, Callback callback) {
    auto order = get_order_helper(order_id);
    if (!order)
      return false;
    callback(*order);
    return true;
  }

  virtual uint64_t get_next_trade_id() = 0;

 private:
  virtual cache::Order *get_order_helper(uint64_t order_id) = 0;
};

}  // namespace algo
}  // namespace roq
