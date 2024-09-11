/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_test_macros.hpp>

#include "roq/algo/matcher/factory.hpp"

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::matcher;

using namespace std::literals;

TEST_CASE("simple", "[algo/matcher]") {
  struct MyDispatcher final : public Dispatcher {
    void operator()(Event<TopOfBook> const &) override {}
    void operator()(Event<MarketByPriceUpdate> const &) override {}
    void operator()(Event<MarketByOrderUpdate> const &) override {}
    void operator()(Event<TradeSummary> const &) override {}
    void operator()(Event<StatisticsUpdate> const &) override {}
    void operator()(Event<CancelAllOrdersAck> const &) override {}
    void operator()(Event<OrderAck> const &) override {}
    void operator()(Event<OrderUpdate> const &) override {}
    void operator()(Event<TradeUpdate> const &) override {}
  } dispatcher;
  struct MyCache final : public Cache {
    cache::Order *get_order_helper([[maybe_unused]] uint64_t order_id) override { return nullptr; }
    uint64_t get_next_trade_id() override { return {}; }
  } cache;
  auto config = Config{
      .source = algo::matcher::Source::TOP_OF_BOOK,
  };
  auto matcher = Factory::create(Factory::Type::SIMPLE, dispatcher, cache, "deribit"sv, "BTC-PERPETUAL"sv, config);
  CHECK((1 + 1) == 2);
}
