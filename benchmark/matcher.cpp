/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <benchmark/benchmark.h>

#include "roq/algo/matcher/factory.hpp"

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::matcher;

using namespace std::literals;

void BM_tools_Simple_add(benchmark::State &state) {
  struct Dispatcher final : public Matcher::Dispatcher {
    void operator()(Event<DownloadBegin> const &) override {}
    void operator()(Event<DownloadEnd> const &) override {}
    void operator()(Event<GatewaySettings> const &) override {}
    void operator()(Event<StreamStatus> const &) override {}
    void operator()(Event<GatewayStatus> const &) override {}
    void operator()(Event<ReferenceData> const &) override {}
    void operator()(Event<MarketStatus> const &) override {}
    void operator()(Event<TopOfBook> const &) override {}
    void operator()(Event<MarketByPriceUpdate> const &) override {}
    void operator()(Event<MarketByOrderUpdate> const &) override {}
    void operator()(Event<TradeSummary> const &) override {}
    void operator()(Event<StatisticsUpdate> const &) override {}
    void operator()(Event<CancelAllOrdersAck> const &) override {}
    void operator()(Event<OrderAck> const &) override {}
    void operator()(Event<OrderUpdate> const &) override {}
    void operator()(Event<TradeUpdate> const &) override {}
    void operator()(Event<PositionUpdate> const &) override {}
    void operator()(Event<FundsUpdate> const &) override {}
  } dispatcher;
  for (auto _ : state) {
    auto matcher = Factory::create(Factory::Type::SIMPLE, dispatcher, "deribit"sv, "BTC-PERPETUAL"sv);
  }
}

BENCHMARK(BM_tools_Simple_add);
