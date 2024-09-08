/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_test_macros.hpp>

#include "roq/algo/matcher/factory.hpp"

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::matcher;

using namespace std::literals;

TEST_CASE("simple", "[algo/matcher]") {
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
  auto matcher = Factory::create(Factory::Type::SIMPLE, dispatcher, "deribit"sv, "BTC-PERPETUAL"sv);
  CHECK((1 + 1) == 2);
}
