/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/algo/matcher/factory.hpp"

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::matcher;

using namespace std::literals;

void BM_tools_Simple_add(benchmark::State &state) {
  struct MyDispatcher final : public Dispatcher {
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
  struct MyCache final : public Cache {
    cache::Order *get_order_helper([[maybe_unused]] uint64_t order_id) override { return nullptr; }
    uint64_t get_next_trade_id() override { return {}; }
  } cache;
  auto config = Config{
      .instrument{
          .source = 0,
          .exchange = "deribit"sv,
          .symbol = "BTC-PERPETUAL"sv,
          .account = {},
      },
      .market_data_source = algo::MarketDataSource::TOP_OF_BOOK,
  };
  for (auto _ : state) {
    auto matcher = Factory::create(Factory::Type::SIMPLE, dispatcher, config, cache);
  }
}

BENCHMARK(BM_tools_Simple_add);
