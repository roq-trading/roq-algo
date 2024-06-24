/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

#include "roq/cache/market_by_price.hpp"

#include "roq/algo/spreader/shared.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Instrument final {
  Instrument(Shared &, std::string_view const &exchange, std::string_view const &symbol, Side side, double total_quantity, double weight);

  bool ready() const { return ready_; }

  void clear();

  void operator()(Event<Timer> const &);

  bool operator()(Event<ReferenceData> const &);
  bool operator()(Event<MarketStatus> const &);
  bool operator()(Event<MarketByPriceUpdate> const &);

  bool operator()(Event<OrderAck> const &);
  bool operator()(Event<OrderUpdate> const &);
  bool operator()(Event<TradeUpdate> const &);
  bool operator()(Event<PositionUpdate> const &);

  bool update_reference_data();
  bool update_market_data();

  double compute_partial_from_impact_price() const { return weight_ * impact_price_; }

  void update(double residual);

  void refresh(std::chrono::nanoseconds now);

  double completed() const { return completion_; }

  void refresh_positions(double completion);

  void DEBUG_print();

 protected:
  enum class State {
    READY,
    CREATING,
    MODIFYING,
  };

  void operator()(State);

  void create_order();
  void modify_price();
  void modify_quantity();

 private:
  Shared &shared_;
  // config
  std::string_view const symbol_;
  Side const side_;
  double const total_quantity_;
  double const weight_;
  double const threshold_quantity_;
  // reference data
  double min_trade_vol_ = NaN;
  double tick_size_ = NaN;
  // market data
  TradingStatus trading_status_ = {};
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
  Layer top_of_book_ = {};
  double impact_price_ = NaN;  // price where we might be able to aggress the remaining quantity
  double target_price_ = NaN;  // price where we want to place an order to at least achieve the target spread
  // status
  bool reference_data_ready_ = {};
  bool market_data_ready_ = {};
  bool ready_ = {};
  // EXPERIMENTAL
  uint64_t order_id_ = {};
  State state_ = {};
  double request_price_ = NaN;
  double order_price_ = NaN;
  std::chrono::nanoseconds next_refresh_ = {};
  double target_quantity_;
  double traded_quantity_ = {};
  double completion_ = {};
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
