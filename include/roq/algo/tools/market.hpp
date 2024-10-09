/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <chrono>
#include <limits>
#include <memory>
#include <string_view>

#include "roq/cache/market_by_order.hpp"
#include "roq/cache/market_by_price.hpp"
#include "roq/cache/market_status.hpp"
#include "roq/cache/top_of_book.hpp"

#include "roq/algo/market_data_source.hpp"

namespace roq {
namespace algo {
namespace tools {

struct ROQ_PUBLIC Market final {
  Market(std::string_view const &exchange, std::string_view const &symbol, MarketDataSource);

  Market(Market &&) = default;
  Market(Market const &) = delete;

  bool has_tick_size() const { return !std::isnan(tick_size_) && precision_ != Precision{}; }

  bool is_market_active(MessageInfo const &, std::chrono::nanoseconds max_age = {}) const;

  std::pair<int64_t, bool> price_to_ticks(double price) const;

  Layer const &get_best() const { return best_; }

  void operator()(Event<ReferenceData> const &);
  void operator()(Event<MarketStatus> const &);

  bool operator()(Event<TopOfBook> const &);
  bool operator()(Event<MarketByPriceUpdate> const &);
  bool operator()(Event<MarketByOrderUpdate> const &);

 private:
  MarketDataSource const market_data_source_;
  double tick_size_ = NaN;
  Precision precision_ = {};
  double multiplier_ = NaN;
  double min_trade_vol_ = NaN;
  cache::MarketStatus market_status_;
  cache::TopOfBook top_of_book_;
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
  std::unique_ptr<cache::MarketByOrder> market_by_order_;
  Layer best_ = {};
  std::chrono::nanoseconds exchange_time_utc_ = {};
  std::chrono::nanoseconds latency_ = {};
};

}  // namespace tools
}  // namespace algo
}  // namespace roq
