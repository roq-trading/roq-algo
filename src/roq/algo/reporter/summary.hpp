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
    // market data
    struct ReferenceData final {
      size_t total_count = {};
    } reference_data;
    struct MarketStatus final {
      size_t total_count = {};
    } market_status;
    struct TopOfBook final {
      size_t total_count = {};
    } top_of_book;
    struct MarketByPriceUpdate final {
      size_t total_count = {};
    } market_by_price_update;
    struct MarketByOrderUpdate final {
      size_t total_count = {};
    } market_by_order_update;
    struct TradeSummary final {
      size_t total_count = {};
    } trade_summary;
    struct StatisticsUpdate final {
      size_t total_count = {};
    } statistics_update;
    // order management
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
    struct PositionUpdate final {
      size_t total_count = {};
      double min_position = NaN;
      double max_position = NaN;
    } position_update;
  };

 protected:
  void print() const override;

  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;
  void operator()(Event<TopOfBook> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<MarketByOrderUpdate> const &) override;
  void operator()(Event<TradeSummary> const &) override;
  void operator()(Event<StatisticsUpdate> const &) override;

  void operator()(Event<OrderAck> const &) override;
  void operator()(Event<OrderUpdate> const &) override;
  void operator()(Event<TradeUpdate> const &) override;
  void operator()(Event<PositionUpdate> const &) override;

  template <typename T, typename Callback>
  void get_instrument(Event<T> const &, Callback);

 private:
  std::vector<utils::unordered_map<std::string, utils::unordered_map<std::string, Instrument>>> instruments_;
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::ReferenceData> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::ReferenceData const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::MarketStatus> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::MarketStatus const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::TopOfBook> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::TopOfBook const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::MarketByPriceUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::MarketByPriceUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::MarketByOrderUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::MarketByOrderUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::TradeSummary> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::TradeSummary const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

template <>
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::StatisticsUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::StatisticsUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={})"
        R"(}})"sv,
        value.total_count);
  }
};

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
struct fmt::formatter<roq::algo::reporter::Summary::Instrument::PositionUpdate> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::reporter::Summary::Instrument::PositionUpdate const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(total_count={}, )"
        R"(min_position={}, )"
        R"(max_position={})"
        R"(}})"sv,
        value.total_count,
        value.min_position,
        value.max_position);
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
        R"(reference_data={}, )"
        R"(market_status={}, )"
        R"(top_of_book={}, )"
        R"(market_by_price_update={}, )"
        R"(market_by_order_update={}, )"
        R"(trade_summary={}, )"
        R"(statistics_update={}, )"
        R"(order_ack={}, )"
        R"(order_update={}, )"
        R"(trade_update={})"
        R"(}})"sv,
        value.reference_data,
        value.market_status,
        value.top_of_book,
        value.market_by_price_update,
        value.market_by_order_update,
        value.trade_summary,
        value.statistics_update,
        value.order_ack,
        value.order_update,
        value.trade_update);
  }
};
