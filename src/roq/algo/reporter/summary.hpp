/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <fmt/core.h>

#include <vector>

#include "roq/utils/container.hpp"

#include "roq/client/reporter.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct Summary final : public client::Reporter {
  struct Instrument final {
    struct OrderAck final {
      size_t accepted_count = {};
      size_t rejected_count = {};
    } order_ack;
    struct OrderUpdate final {
      size_t buy_count = {};
      size_t sell_count = {};
      size_t total_count = {};
    } order_update;
    struct TradeUpdate final {
      struct Fills final {
        size_t buy_count = {};
        size_t sell_count = {};
        size_t total_count = {};
        double buy_volume = 0.0;
        double sell_volume = 0.0;
        double total_volume = 0.0;
      } fills;
    } trade_update;
  };

 protected:
  void print() const override;

  void operator()(Event<OrderAck> const &) override;
  void operator()(Event<OrderUpdate> const &) override;
  void operator()(Event<TradeUpdate> const &) override;

  template <typename T, typename Callback>
  void get_instrument(Event<T> const &, Callback);

 private:
  std::vector<utils::unordered_map<std::string, utils::unordered_map<std::string, Instrument>>> instruments_;
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::OrderAck> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::OrderAck const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(accepted_count={}, )"
        R"(rejected_count={})"
        R"(}})"sv,
        value.accepted_count,
        value.rejected_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::OrderUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::OrderUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(buy_count={}, )"
        R"(sell_count={}, )"
        R"(total_count={})"
        R"(}})"sv,
        value.buy_count,
        value.sell_count,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::TradeUpdate::Fills> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::TradeUpdate::Fills const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(buy_count={}, )"
        R"(sell_count={}, )"
        R"(total_count={}, )"
        R"(buy_volume={}, )"
        R"(sell_volume={}, )"
        R"(total_volume={})"
        R"(}})"sv,
        value.buy_count,
        value.sell_count,
        value.total_count,
        value.buy_volume,
        value.sell_volume,
        value.total_volume);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::TradeUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::TradeUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(fills={})"
        R"(}})"sv,
        value.fills);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(order_ack={}, )"
        R"(order_update={}, )"
        R"(trade_update={})"
        R"(}})"sv,
        value.order_ack,
        value.order_update,
        value.trade_update);
  }
};
