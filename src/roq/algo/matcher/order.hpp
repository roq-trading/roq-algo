/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string>

#include "roq/cache/order.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct Order final : public cache::Order {
  explicit Order(Event<CreateOrder> const &);

  operator OrderUpdate() const;

  void operator()(Fill const &);

  // void operator()(double traded_quantity, double remaining_quantity, double last_traded_price, double last_traded_quantity, OrderStatus, Liquidity);

  std::string const account;
  std::string const exchange;
  std::string const symbol;

 private:
  double total_cost_ = 0.0;
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
