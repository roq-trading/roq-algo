/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <fmt/format.h>

#include <magic_enum.hpp>

#include <string>

#include "roq/api.hpp"

#include "roq/algo/instrument.hpp"
#include "roq/algo/market_data_source.hpp"

#include "roq/algo/tools/market.hpp"

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
  Instrument(algo::Instrument const &, MarketDataSource);

  bool is_market_active(MessageInfo const &message_info, std::chrono::nanoseconds max_age) const { return market_.is_market_active(message_info, max_age); }

  Layer const &get_best() const { return market_.get_best(); }

  void operator()(Event<ReferenceData> const &event) { market_(event); }
  void operator()(Event<MarketStatus> const &event) { market_(event); }

  void operator()(Event<TopOfBook> const &event) { market_(event); }
  void operator()(Event<MarketByPriceUpdate> const &event) { market_(event); }
  void operator()(Event<MarketByOrderUpdate> const &event) { market_(event); }

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
        R"(order_state={}, )"
        R"(order_id={}, )"
        R"(position={}, )"
        R"(market={})"
        R"(}})"sv,
        source,
        exchange,
        symbol,
        account,
        magic_enum::enum_name(order_state),
        order_id,
        position,
        market_);
  }

  uint8_t const source = {};
  std::string const exchange;
  std::string const symbol;
  std::string const account;
  OrderState order_state = {};
  uint64_t order_id = {};
  double position = 0.0;

 private:
  tools::Market market_;
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::arbitrage::Instrument> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::arbitrage::Instrument const &value, format_context &context) const { return value.format_helper(context.out()); }
};
