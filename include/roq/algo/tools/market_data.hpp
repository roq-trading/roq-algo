/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/format.h>

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

struct ROQ_PUBLIC MarketData final {
  MarketData(std::string_view const &exchange, std::string_view const &symbol, MarketDataSource);

  MarketData(MarketData &&) = default;
  MarketData(MarketData const &) = delete;

  bool has_tick_size() const { return !std::isnan(tick_size_) && precision_ != Precision{}; }

  bool is_market_active(MessageInfo const &, std::chrono::nanoseconds max_age = {}) const;

  std::pair<int64_t, bool> price_to_ticks(double price) const;

  // note! depends on MarketDataSource
  Layer const &top_of_book() const { return best_; }

  std::chrono::nanoseconds exchange_time_utc() const { return exchange_time_utc_; }

  bool operator()(Event<ReferenceData> const &);
  bool operator()(Event<MarketStatus> const &);

  // note! depends on MarketDataSource
  bool operator()(Event<TopOfBook> const &);
  bool operator()(Event<MarketByPriceUpdate> const &);
  bool operator()(Event<MarketByOrderUpdate> const &);

  void operator()(Event<TradeSummary> const &);
  void operator()(Event<StatisticsUpdate> const &);

  template <typename OutputIt>
  auto constexpr format_helper(OutputIt out) const {
    using namespace std::literals;
    return fmt::format_to(
        out,
        R"({{)"
        R"(tick_size={}, )"
        R"(multiplier={}, )"
        R"(min_trade_vol={}, )"
        R"(trading_status={}, )"
        R"(best={}, )"
        R"(exchange_time_utc={}, )"
        R"(latency={})"
        R"(}})"sv,
        tick_size_,
        multiplier_,
        min_trade_vol_,
        market_status_.trading_status,
        best_,
        exchange_time_utc_,
        latency_);
  }

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

template <>
struct fmt::formatter<roq::algo::tools::MarketData> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::tools::MarketData const &value, format_context &context) const { return value.format_helper(context.out()); }
};
