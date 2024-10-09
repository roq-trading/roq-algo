/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <vector>

#include "roq/utils/container.hpp"

#include "roq/algo/market_data_source.hpp"
#include "roq/algo/order_cache.hpp"

#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"

#include "roq/algo/arbitrage/config.hpp"
#include "roq/algo/arbitrage/instrument.hpp"

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

struct Simple final : public strategy::Handler {
  using Dispatcher = strategy::Dispatcher;

  Simple(Dispatcher &, Config const &, OrderCache &);

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

  // utils

  template <typename Callback>
  void get_instruments_by_source(MessageInfo const &, Callback);

  template <typename T, typename Callback>
  bool get_instrument(Event<T> const &, Callback);

  template <typename T>
  void update_latency(std::chrono::nanoseconds &latency, Event<T> const &);

  void update(MessageInfo const &);

  bool is_ready(MessageInfo const &, Instrument const &) const;

  bool can_trade(Side, Instrument &);

  void maybe_trade(MessageInfo const &, Side, Instrument &lhs, Instrument &rhs);

  struct Order final {
    Side side = {};
    double quantity = NaN;
    OrderStatus order_status = {};
  };

  struct Account final {
    bool has_download_orders = {};
    utils::unordered_map<size_t, Order> working_orders_by_instrument;  // <<< maybe in instrument
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

 private:
  Dispatcher &dispatcher_;
  // config
  uint32_t const strategy_id_;
  std::chrono::nanoseconds const max_age_;  // used when trading status is unavailable
  SupportType const market_data_type_;
  double const threshold_;
  double const quantity_0_;
  double const min_position_0_;
  double const max_position_0_;
  // cache
  OrderCache &order_cache_;
  // state
  std::vector<Instrument> instruments_;
  std::vector<Source> sources_;
  uint64_t max_order_id_ = {};
  // DEBUG
  std::chrono::nanoseconds last_receive_time_ = {};
  std::chrono::nanoseconds last_receive_time_utc_ = {};
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
