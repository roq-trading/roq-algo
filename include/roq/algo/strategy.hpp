/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include "roq/api.hpp"

#include "roq/metrics/writer.hpp"

#include "roq/cache/order.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Strategy {
  struct ROQ_PUBLIC Dispatcher {
    virtual void operator()(ControlAck const &, uint8_t source) = 0;
    virtual void operator()(ServiceUpdate const &) = 0;
    virtual void operator()(StrategyUpdate const &) = 0;
    virtual void operator()(LegsUpdate const &) = 0;

    virtual void send(CreateOrder const &, uint8_t source, bool is_last = true) = 0;
    virtual void send(ModifyOrder const &, uint8_t source, bool is_last = true) = 0;
    virtual void send(CancelOrder const &, uint8_t source, bool is_last = true) = 0;

    virtual void send(CancelAllOrders const &, uint8_t source) = 0;

    virtual void send(MassQuote const &, uint8_t source) = 0;
    virtual void send(CancelQuotes const &, uint8_t source) = 0;

    virtual void send(CustomMetrics const &, uint8_t source) = 0;
    virtual void send(CustomMatrix const &, uint8_t source) = 0;
  };

  virtual ~Strategy() = default;

  // host
  virtual void operator()(Event<Start> const &) {}
  virtual void operator()(Event<Stop> const &) {}
  virtual void operator()(Event<Timer> const &) {}

  // connection
  virtual void operator()(Event<Connected> const &) {}
  virtual void operator()(Event<Disconnected> const &) {}

  // control
  virtual void operator()(Event<Control> const &) {};

  // download
  virtual void operator()(Event<DownloadBegin> const &) {}
  virtual void operator()(Event<DownloadEnd> const &) {}
  virtual void operator()(Event<Ready> const &) {}

  // config
  virtual void operator()(Event<GatewaySettings> const &) {}

  // stream
  virtual void operator()(Event<StreamStatus> const &) {}
  virtual void operator()(Event<ExternalLatency> const &) {}
  virtual void operator()(Event<RateLimitsUpdate> const &) {}
  virtual void operator()(Event<RateLimitTrigger> const &) {}

  // service
  virtual void operator()(Event<GatewayStatus> const &) {}

  // market data
  virtual void operator()(Event<ReferenceData> const &) {}
  virtual void operator()(Event<MarketStatus> const &) {}
  virtual void operator()(Event<TopOfBook> const &) {}
  virtual void operator()(Event<MarketByPriceUpdate> const &) {}
  virtual void operator()(Event<MarketByOrderUpdate> const &) {}
  virtual void operator()(Event<TradeSummary> const &) {}
  virtual void operator()(Event<StatisticsUpdate> const &) {}

  // market data (derived)
  virtual void operator()(Event<TimeSeriesUpdate> const &) {}

  // order management
  virtual void operator()(Event<CancelAllOrdersAck> const &) {}
  virtual void operator()(Event<OrderAck> const &, cache::Order const &) {}
  virtual void operator()(Event<OrderUpdate> const &, cache::Order const &) {}
  virtual void operator()(Event<TradeUpdate> const &, cache::Order const &) {}

  // account management (exchange)
  virtual void operator()(Event<PositionUpdate> const &) {}
  virtual void operator()(Event<FundsUpdate> const &) {}

  // broadcast
  virtual void operator()(Event<CustomMetricsUpdate> const &) {}
  virtual void operator()(Event<CustomMatrixUpdate> const &) {}

  // parameters
  virtual void operator()(Event<ParametersUpdate> const &) {}

  // position manager (service)
  virtual void operator()(Event<PortfolioUpdate> const &) {}

  // risk managemenet
  virtual void operator()(Event<RiskLimitsUpdate> const &) {}

  // market making
  virtual void operator()(Event<MassQuoteAck> const &) {}
  virtual void operator()(Event<CancelQuotesAck> const &) {}

  // metrics
  virtual void operator()(metrics::Writer &) const {}
};

}  // namespace algo
}  // namespace roq
