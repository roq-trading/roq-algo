/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

#include "roq/cache/order.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Handler {
  virtual ~Handler() {}

  virtual void operator()(Event<ReferenceData> const &) = 0;
  virtual void operator()(Event<MarketStatus> const &) = 0;

  virtual void operator()(Event<TopOfBook> const &) = 0;
  virtual void operator()(Event<MarketByPriceUpdate> const &) = 0;
  virtual void operator()(Event<MarketByOrderUpdate> const &) = 0;
  virtual void operator()(Event<TradeSummary> const &) = 0;
  virtual void operator()(Event<StatisticsUpdate> const &) = 0;

  virtual void operator()(Event<CreateOrder> const &, cache::Order &) = 0;
  virtual void operator()(Event<ModifyOrder> const &, cache::Order &) = 0;
  virtual void operator()(Event<CancelOrder> const &, cache::Order &) = 0;

  virtual void operator()(Event<CancelAllOrders> const &) = 0;
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
