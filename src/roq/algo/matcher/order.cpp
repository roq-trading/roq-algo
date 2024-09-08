/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/matcher/order.hpp"

#include "roq/logging.hpp"

#include "roq/utils/compare.hpp"

namespace roq {
namespace algo {
namespace matcher {

// === IMPLEMENTATION ===

Order::Order(Event<CreateOrder> const &event) : cache::Order{event}, account{event.value.account}, exchange{event.value.exchange}, symbol{event.value.symbol} {
}

Order::operator OrderUpdate() const {
  return convert(*this);
}

void Order::operator()(Fill const &) {
}

/*
void Order::operator()(
    double traded_quantity, double remaining_quantity, double last_traded_price, double last_traded_quantity, OrderStatus order_status, Liquidity liquidity) {
  // order
  (*this).order_status = order_status;
  (*this).remaining_quantity = remaining_quantity;
  // total
  if (utils::is_greater(last_traded_quantity, 0.0)) {
    (*this).traded_quantity = traded_quantity;
    total_cost_ += last_traded_price * last_traded_quantity;
    (*this).average_traded_price = total_cost_ / traded_quantity;
  }
  // last
  (*this).last_traded_quantity = last_traded_quantity;
  (*this).last_traded_price = last_traded_price;
  (*this).last_liquidity = liquidity;
}
*/

}  // namespace matcher
}  // namespace algo
}  // namespace roq
