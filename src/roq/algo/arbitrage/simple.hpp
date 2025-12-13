/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/utils/container.hpp"

#include "roq/algo/market_data_source.hpp"
#include "roq/algo/order_cache.hpp"

#include "roq/algo/tools/time_checker.hpp"

#include "roq/algo/strategy.hpp"

#include "roq/algo/strategy/config.hpp"

#include "roq/algo/arbitrage/instrument.hpp"
#include "roq/algo/arbitrage/parameters.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

// simple arbitrage
//
// prepared to support a list of instruments (n >= 2)
//
// assumptions:
// - only supporting positions (*not* FX-style)
//
// Q:
// - trading size
// - scaling factor?
// - max position
// - latency
// - liquid vs illiquid leg?
// - multiplier (compare real size)
// - threshold

struct Simple final : public Strategy {
  Simple(Dispatcher &, OrderCache &, strategy::Config const &, Parameters const &);

  Simple(Simple &&) = delete;
  Simple(Simple const &) = delete;

 protected:
  void operator()(Event<Timer> const &) override;

  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;

  void operator()(Event<DownloadEnd> const &) override;

  void operator()(Event<Ready> const &) override;

  void operator()(Event<StreamStatus> const &) override;
  void operator()(Event<ExternalLatency> const &) override;

  void operator()(Event<GatewayStatus> const &) override;

  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;

  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<MarketByOrderUpdate> const &) override;

  void operator()(Event<OrderAck> const &, cache::Order const &) override;
  void operator()(Event<OrderUpdate> const &, cache::Order const &) override;
  void operator()(Event<TradeUpdate> const &, cache::Order const &) override;

  void operator()(Event<PositionUpdate> const &) override;
  void operator()(Event<FundsUpdate> const &) override;

  void operator()(Event<PortfolioUpdate> const &) override;

  // utils

  template <typename Callback>
  void get_instruments_by_source(MessageInfo const &, Callback);

  template <typename T, typename Callback>
  bool get_instrument(Event<T> const &, Callback);

  template <typename T>
  void update_latency(std::chrono::nanoseconds &latency, Event<T> const &);

  void update(MessageInfo const &);

  void check_spread(MessageInfo const &, Instrument &lhs, Instrument &rhs);

  void maybe_trade_spread(MessageInfo const &, Side, Instrument &lhs, Instrument &rhs);

  bool can_trade(Side, Instrument &) const;

  struct Order final {
    Side side = {};
    double quantity = NaN;
    OrderStatus order_status = {};
  };

  struct Account final {
    bool has_download_orders = {};
    utils::unordered_map<size_t, Order> working_orders_by_instrument;  // XXX FIXME TODO maybe move to instrument?
  };

  struct Source final {
    utils::unordered_map<std::string_view, Account> accounts;
    utils::unordered_map<std::string_view, utils::unordered_map<std::string_view, size_t>> const instruments;
    bool ready = {};
    std::vector<std::chrono::nanoseconds> stream_latency;
    utils::unordered_map<uint64_t, Order> working_orders;
  };

  template <typename T, typename Callback>
  bool get_account_and_instrument(Event<T> const &, Callback);

  template <typename T>
  void check(Event<T> const &);

  template <typename T>
  bool is_mine(Event<T> const &) const;

  void publish_statistics(Instrument &);

 private:
  Dispatcher &dispatcher_;
  uint32_t const strategy_id_;
  std::chrono::nanoseconds const max_age_;  // used when exchange doesn't support trading status
  double const threshold_;                  // abs(spread) must exceed this threshold
  double const quantity_0_;
  double const min_position_0_;
  double const max_position_0_;
  uint8_t const publish_source_;
  SupportType const market_data_type_;
  OrderCache &order_cache_;
  std::vector<Instrument> instruments_;
  std::vector<Source> sources_;
  uint64_t max_order_id_ = {};
  // DEBUG
  tools::TimeChecker time_checker_;
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
