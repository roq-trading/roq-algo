/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Dispatcher {
  /*
  virtual void operator()(Event<DownloadBegin> const &) = 0;
  virtual void operator()(Event<DownloadEnd> const &) = 0;

  // config
  virtual void operator()(Event<GatewaySettings> const &) = 0;

  // stream
  virtual void operator()(Event<StreamStatus> const &) = 0;

  // service
  virtual void operator()(Event<GatewayStatus> const &) = 0;

  virtual void operator()(Event<ReferenceData> const &) = 0;
  virtual void operator()(Event<MarketStatus> const &) = 0;
  */

  virtual void operator()(Event<TopOfBook> const &) = 0;
  virtual void operator()(Event<MarketByPriceUpdate> const &) = 0;
  virtual void operator()(Event<MarketByOrderUpdate> const &) = 0;
  virtual void operator()(Event<TradeSummary> const &) = 0;
  virtual void operator()(Event<StatisticsUpdate> const &) = 0;

  // order management
  virtual void operator()(Event<CancelAllOrdersAck> const &) = 0;
  virtual void operator()(Event<OrderAck> const &) = 0;
  virtual void operator()(Event<OrderUpdate> const &) = 0;
  virtual void operator()(Event<TradeUpdate> const &) = 0;

  /*
  // account management
  virtual void operator()(Event<PositionUpdate> const &) = 0;
  virtual void operator()(Event<FundsUpdate> const &) = 0;
  */
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
