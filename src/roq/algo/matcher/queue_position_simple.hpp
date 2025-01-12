/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <limits>
#include <vector>

#include "roq/algo/market_data_source.hpp"
#include "roq/algo/order_cache.hpp"

#include "roq/algo/tools/market_data.hpp"
#include "roq/algo/tools/time_checker.hpp"

#include "roq/algo/matcher.hpp"

#include "roq/algo/matcher/config.hpp"

namespace roq {
namespace algo {
namespace matcher {

// queue position simple matcher

struct QueuePositionSimple final : public Matcher {
  using Dispatcher = Matcher::Dispatcher;

  QueuePositionSimple(Dispatcher &, OrderCache &, Config const &);

  QueuePositionSimple(QueuePositionSimple const &) = delete;

 protected:
  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;

  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<MarketByOrderUpdate> const &) override;
  void operator()(Event<TradeSummary> const &) override;
  void operator()(Event<StatisticsUpdate> const &) override;

  void operator()(Event<CreateOrder> const &, cache::Order &) override;
  void operator()(Event<ModifyOrder> const &, cache::Order &) override;
  void operator()(Event<CancelOrder> const &, cache::Order &) override;

  void operator()(Event<CancelAllOrders> const &) override;

  void operator()(Event<MassQuote> const &) override;
  void operator()(Event<CancelQuotes> const &) override;

  // market

  void match_resting_orders(MessageInfo const &);
  void match_resting_orders_2(Event<TradeSummary> const &);

  // orders

  template <typename T>
  void dispatch_order_ack(Event<T> const &, cache::Order const &, Error, RequestStatus = {});

  void dispatch_order_update(MessageInfo const &, cache::Order &);

  void dispatch_trade_update(MessageInfo const &, cache::Order const &, Fill const &);

  // utils

  bool is_aggressive(Side, int64_t price) const;

  void add_order(uint64_t order_id, Side, int64_t price);

  bool remove_order(uint64_t order_id, Side, int64_t price);

  template <typename Callback>
  void try_match(Side, Callback);

  template <typename T>
  void check(Event<T> const &);

 private:
  Dispatcher &dispatcher_;
  OrderCache &order_cache_;
  tools::MarketData market_data_;
  // note! internal (integer) is in units of tick_size, external (floating point) is the real price
  struct {
    struct {
      int64_t bid_price = std::numeric_limits<int64_t>::min();
      int64_t ask_price = std::numeric_limits<int64_t>::max();
    } internal;
    struct {
      double bid_price = NaN;
      double ask_price = NaN;
    } external;
  } top_of_book_;
  struct Order final {
    uint64_t order_id = {};
    int64_t price = {};
    double ahead = NaN;  // note! could change this to uint64_t
  };
  std::vector<Order> buy_orders_;
  std::vector<Order> sell_orders_;
  // DEBUG
  tools::TimeChecker time_checker_;
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
