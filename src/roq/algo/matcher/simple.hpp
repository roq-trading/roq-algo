/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <limits>
#include <string>
#include <vector>

#include "roq/matcher.hpp"

#include "roq/utils/container.hpp"

#include "roq/cache/market_by_order.hpp"
#include "roq/cache/market_by_price.hpp"
#include "roq/cache/market_status.hpp"
#include "roq/cache/top_of_book.hpp"

#include "roq/algo/matcher/config.hpp"

#include "roq/algo/matcher/order.hpp"

namespace roq {
namespace algo {
namespace matcher {

// simple matcher
//
// placing a new order
// - immediately filled if price cross the market (opposite side)
// - leaves a resting order if price does not cross the market
//
// best market update
// - fills those orders crossing the market

struct Simple final : public Matcher {
  Simple(Matcher::Dispatcher &, std::string_view const &exchange, std::string_view const &symbol, Config const &);

  Simple(Simple const &) = delete;

  void operator()(Event<GatewaySettings> const &) override;

  void operator()(Event<StreamStatus> const &) override;

  void operator()(Event<GatewayStatus> const &) override;

  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;
  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<MarketByOrderUpdate> const &) override;
  void operator()(Event<TradeSummary> const &) override;
  void operator()(Event<StatisticsUpdate> const &) override;

  void operator()(Event<PositionUpdate> const &) override;
  void operator()(Event<FundsUpdate> const &) override;

  void operator()(Event<CreateOrder> const &) override;
  void operator()(Event<ModifyOrder> const &) override;
  void operator()(Event<CancelOrder> const &) override;
  void operator()(Event<CancelAllOrders> const &) override;

 protected:
  void operator()(Event<Layer> const &);

  template <typename T, typename Callback>
  bool find_order(Event<T> const &, Callback);

  template <typename T>
  void dispatch_order_ack(Event<T> const &, Error, RequestStatus = {});

  template <typename T>
  void dispatch_order_ack(Event<T> const &, Order const &, Error, RequestStatus = {});

  void dispatch_order_update(MessageInfo const &, Order const &, UpdateType);

  void dispatch_trade_update(MessageInfo const &, Order const &, Fill const &);

  bool is_aggressive(Side, int64_t price) const;

  void add_order(uint64_t order_id, Side, int64_t price);

  bool remove_order(uint64_t order_id, Side, int64_t price);

  template <typename Callback>
  void try_match(Side, Callback);

 private:
  Matcher::Dispatcher &dispatcher_;
  // config
  std::string const exchange_;
  std::string const symbol_;
  Config const config_;
  // market
  std::chrono::nanoseconds exchange_time_utc_ = {};
  double tick_size_ = NaN;
  Precision precision_ = {};
  cache::MarketStatus market_status_;
  cache::TopOfBook top_of_book_;
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
  std::unique_ptr<cache::MarketByOrder> market_by_order_;
  std::pair<int64_t, int64_t> best_internal_ = {
      std::numeric_limits<int64_t>::min(),
      std::numeric_limits<int64_t>::max(),
  };
  std::pair<double, double> best_external_ = {NaN, NaN};
  // account
  utils::unordered_set<std::string> accounts_;
  // orders
  uint64_t max_order_id_ = {};
  utils::unordered_map<uint64_t, Order> orders_;
  std::vector<std::pair<int64_t, uint64_t>> buy_;
  std::vector<std::pair<int64_t, uint64_t>> sell_;
  // trades
  uint64_t trade_id_ = {};
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
