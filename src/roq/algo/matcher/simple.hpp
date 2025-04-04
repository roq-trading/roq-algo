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

// simple matcher
//
// placing a new order
// - price crossing market best => immediately filled
// - price not crossing market best => leaves a resting order
//
// market best updates
// - fills any resting orders crossing market best
//
// supports
// - limit orders

struct Simple final : public Matcher {
  using Dispatcher = Matcher::Dispatcher;

  Simple(Dispatcher &, OrderCache &, Config const &);

  Simple(Simple const &) = delete;

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
    std::pair<int64_t, int64_t> internal = {
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
    };
    std::pair<double, double> external = {NaN, NaN};
  } top_of_book_;
  // note! priority is preserved by first ordering by price (internal) and then by order_id
  std::vector<std::pair<int64_t, uint64_t>> buy_orders_;
  std::vector<std::pair<int64_t, uint64_t>> sell_orders_;
  // DEBUG
  tools::TimeChecker time_checker_;
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
