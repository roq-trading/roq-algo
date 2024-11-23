/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/algo/matcher/factory.hpp"

using namespace std::literals;

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::matcher;

void BM_tools_Simple_add(benchmark::State &state) {
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
  for (auto _ : state) {
    auto matcher = Factory::create(Type::SIMPLE, dispatcher, order_cache, config);
  }
}

BENCHMARK(BM_tools_Simple_add);
