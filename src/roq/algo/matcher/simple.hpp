/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <limits>
#include <vector>

#include "roq/cache/market_by_order.hpp"
#include "roq/cache/market_by_price.hpp"
#include "roq/cache/market_status.hpp"
#include "roq/cache/top_of_book.hpp"

#include "roq/algo/matcher/cache.hpp"
#include "roq/algo/matcher/config.hpp"
#include "roq/algo/matcher/dispatcher.hpp"
#include "roq/algo/matcher/handler.hpp"

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

struct Simple final : public Handler {
  Simple(Dispatcher &, Cache &, std::string_view const &exchange, std::string_view const &symbol, Config const &);

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

  // market

  void operator()(Event<Layer> const &);

  // orders

  template <typename T>
  void dispatch_order_ack(Event<T> const &, cache::Order const &, Error, RequestStatus = {});

  void dispatch_order_update(MessageInfo const &, cache::Order const &, UpdateType);

  void dispatch_trade_update(MessageInfo const &, cache::Order const &, Fill const &);

  // utils

  bool is_aggressive(Side, int64_t price) const;

  void add_order(uint64_t order_id, Side, int64_t price);

  bool remove_order(uint64_t order_id, Side, int64_t price);

  template <typename Callback>
  void try_match(Side, Callback);

 private:
  Dispatcher &dispatcher_;
  Cache &cache_;
  Config const config_;
  // market
  std::chrono::nanoseconds exchange_time_utc_ = {};
  double tick_size_ = NaN;
  Precision precision_ = {};
  cache::MarketStatus market_status_;
  cache::TopOfBook top_of_book_;
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
  std::unique_ptr<cache::MarketByOrder> market_by_order_;
  struct {
    std::pair<int64_t, int64_t> units = {
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
    };
    std::pair<double, double> external = {NaN, NaN};
  } best_;
  // orders
  std::vector<std::pair<int64_t, uint64_t>> buy_;
  std::vector<std::pair<int64_t, uint64_t>> sell_;
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
