/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

namespace roq {
namespace algo {
namespace strategy {

struct ROQ_PUBLIC Dispatcher {
  virtual void send(CreateOrder const &, uint8_t source, bool is_last = true) = 0;
  virtual void send(ModifyOrder const &, uint8_t source, bool is_last = true) = 0;
  virtual void send(CancelOrder const &, uint8_t source, bool is_last = true) = 0;

  virtual void send(CancelAllOrders const &, uint8_t source) = 0;
  virtual uint8_t broadcast(CancelAllOrders const &) = 0;
};

}  // namespace strategy
}  // namespace algo
}  // namespace roq
