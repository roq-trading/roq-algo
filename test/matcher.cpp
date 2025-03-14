/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_test_macros.hpp>

#include "roq/algo/matcher/factory.hpp"

using namespace std::literals;

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::matcher;

TEST_CASE("algo_matcher_simple", "[algo_matcher]") {
  struct MyDispatcher final : public Matcher::Dispatcher {
    void operator()(Event<ReferenceData> const &) override {}
    void operator()(Event<MarketStatus> const &) override {}
    void operator()(Event<TopOfBook> const &) override {}
    void operator()(Event<MarketByPriceUpdate> const &) override {}
    void operator()(Event<MarketByOrderUpdate> const &) override {}
    void operator()(Event<TradeSummary> const &) override {}
    void operator()(Event<StatisticsUpdate> const &) override {}
    void operator()(Event<OrderAck> const &) override {}
    void operator()(Event<OrderUpdate> const &) override {}
    void operator()(Event<TradeUpdate> const &) override {}
    void operator()(Event<MassQuoteAck> const &) override {}
    void operator()(Event<CancelQuotesAck> const &) override {}
  } dispatcher;
  struct MyOrderCache final : public OrderCache {
    cache::Order *get_order_helper([[maybe_unused]] uint64_t order_id) override { return nullptr; }
    uint64_t get_next_trade_id() override { return {}; }
  } order_cache;
  auto config = Config{
      .exchange = "deribit"sv,
      .symbol = "BTC-PERPETUAL"sv,
      .market_data_source = algo::MarketDataSource::TOP_OF_BOOK,
  };
  auto matcher = Factory::create(Type::SIMPLE, dispatcher, order_cache, config);
  CHECK((1 + 1) == 2);
}
