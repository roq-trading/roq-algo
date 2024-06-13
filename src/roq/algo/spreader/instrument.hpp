/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

#include "roq/cache/market_by_price.hpp"

#include "roq/algo/spreader/shared.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Instrument final {
  Instrument(Shared &, std::string_view const &exchange, std::string_view const &symbol, Side side, double total_quantity, double weight, double target_spread);

  bool ready() const { return ready_; }

  void clear();

  bool operator()(Event<ReferenceData> const &);
  bool operator()(Event<MarketStatus> const &);
  bool operator()(Event<MarketByPriceUpdate> const &);

  void operator()(Event<OrderAck> const &);
  void operator()(Event<OrderUpdate> const &);
  void operator()(Event<TradeUpdate> const &);
  void operator()(Event<PositionUpdate> const &);

  bool update_reference_data();
  bool update_market_data();

  double value() const {
    // contribution when computing the residual
    return weight_ * impact_price_;
  }

  void update(double residual);

  void refresh();

  void DEBUG_print();

 private:
  Shared &shared_;
  // config
  std::string_view const symbol_;
  Side const side_;
  double const total_quantity_;
  double const weight_;
  double const target_spread_;
  // reference data
  double min_trade_vol_ = std::numeric_limits<double>::quiet_NaN();
  double tick_size_ = std::numeric_limits<double>::quiet_NaN();
  // market data
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
  Layer top_of_book_ = {};
  double impact_price_ = std::numeric_limits<double>::quiet_NaN();  // aggressive
  double target_price_ = std::numeric_limits<double>::quiet_NaN();  // passive
  // status
  bool reference_data_ready_ = {};
  bool market_data_ready_ = {};
  bool ready_ = {};
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
