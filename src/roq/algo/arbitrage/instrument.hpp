/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <magic_enum/magic_enum_format.hpp>

#include <string>
#include <utility>

#include "roq/api.hpp"

#include "roq/algo/leg.hpp"
#include "roq/algo/market_data_source.hpp"

#include "roq/algo/tools/market_data.hpp"
#include "roq/algo/tools/position_tracker.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

enum class OrderState {
  IDLE,
  CREATE,
  WORKING,
  CANCEL,
};

struct Instrument final {
  Instrument(Leg const &, MarketDataSource);

  // market data

  Layer const &top_of_book() const { return market_data_.top_of_book(); }

  std::pair<double, double> get_best() const {
    auto &top_of_book = market_data_.top_of_book();
    return {top_of_book.bid_price, top_of_book.ask_price};
  }

  bool is_ready(MessageInfo const &, std::chrono::nanoseconds max_age) const;

  // order management

  void reset();

  double current_position() const { return position_tracker_.current_position(); }

  // events

  void operator()(Event<Disconnected> const &) { reset(); }

  bool operator()(Event<ReferenceData> const &event) { return market_data_(event); }
  bool operator()(Event<MarketStatus> const &event) { return market_data_(event); }
  bool operator()(Event<TopOfBook> const &event) { return market_data_(event); }
  bool operator()(Event<MarketByPriceUpdate> const &event) { return market_data_(event); }
  bool operator()(Event<MarketByOrderUpdate> const &event) { return market_data_(event); }

  void operator()(Event<TradeUpdate> const &event) { position_tracker_(event); }
  void operator()(Event<PositionUpdate> const &event) { position_tracker_(event); }

  template <typename OutputIt>
  auto constexpr format_helper(OutputIt out) const {
    using namespace std::literals;
    return fmt::format_to(
        out,
        R"({{)"
        R"(source={}, )"
        R"(exchange="{}", )"
        R"(symbol="{}", )"
        R"(account="{}", )"
        R"(market_data={}, )"
        R"(position_tracker={}, )"
        R"(order_state={}, )"
        R"(order_id={})"
        R"(}})"sv,
        source,
        exchange,
        symbol,
        account,
        market_data_,
        position_tracker_,
        order_state,
        order_id);
  }

 protected:
  bool is_market_active(MessageInfo const &message_info, std::chrono::nanoseconds max_age) const {
    return market_data_.is_market_active(message_info, max_age);
  }

 public:
  uint8_t const source = {};
  std::string const exchange;
  std::string const symbol;
  std::string const account;
  PositionEffect const position_effect;
  MarginMode const margin_mode;
  TimeInForce const time_in_force;

 private:
  tools::MarketData market_data_;
  tools::PositionTracker position_tracker_;

 public:
  OrderState order_state = {};
  uint64_t order_id = {};
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::arbitrage::Instrument> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::arbitrage::Instrument const &value, format_context &context) const { return value.format_helper(context.out()); }
};
