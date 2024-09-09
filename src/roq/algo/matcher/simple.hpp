/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string>
#include <vector>

#include "roq/matcher.hpp"

#include "roq/utils/container.hpp"

#include "roq/cache/market_by_order.hpp"
#include "roq/cache/market_by_price.hpp"
#include "roq/cache/market_status.hpp"
#include "roq/cache/top_of_book.hpp"

#include "roq/algo/matcher/order.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct Simple final : public Matcher {
  enum class MatchingSource {
    TOP_OF_BOOK,
    MARKET_BY_PRICE,
    MARKET_BY_ORDER,
  };

  Simple(Matcher::Dispatcher &, std::string_view const &exchange, std::string_view const &symbol, MatchingSource);

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
  void operator()(Layer const &);

  template <typename T, typename Callback>
  bool find_order(Event<T> const &, Callback);

  template <typename T>
  void dispatch_order_ack(Event<T> const &, Error, RequestStatus = {});

  template <typename T>
  void dispatch_order_ack(Event<T> const &, Order const &, Error, RequestStatus = {});

  void dispatch_order_update(MessageInfo const &, Order const &);

  void dispatch_trade_update(MessageInfo const &, Order const &, Fill const &);

  bool is_aggressive(Side, int64_t price) const;

  void add_order(uint64_t order_id, Side, int64_t price);
  void remove_order(uint64_t order_id, Side, int64_t price);

  void try_match();

 private:
  Matcher::Dispatcher &dispatcher_;
  // config
  std::string const exchange_;
  std::string const symbol_;
  MatchingSource const matching_source_;
  // market
  double tick_size_ = NaN;
  Precision precision_ = {};
  cache::MarketStatus market_status_;
  cache::TopOfBook top_of_book_;
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
  std::unique_ptr<cache::MarketByOrder> market_by_order_;
  std::pair<int64_t, int64_t> best_ = {};
  // account
  utils::unordered_set<std::string> accounts_;
  // order
  uint64_t max_order_id_ = {};
  utils::unordered_map<uint64_t, Order> order_;
  std::vector<std::pair<int64_t, uint64_t>> buy_;
  std::vector<std::pair<int64_t, uint64_t>> sell_;
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
