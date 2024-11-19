/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <chrono>
#include <string_view>

#include "roq/api.hpp"

#include "roq/variant_type.hpp"

#include "roq/algo/reporter/output_type.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Reporter {
  enum class Type {
    INDEX,
    DATA,
  };

  struct Handler {
    virtual void operator()(std::string_view const &name, Type, std::span<std::string_view const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<bool const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<uint8_t const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<uint32_t const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<uint64_t const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<double const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<std::chrono::nanoseconds const> const &) = 0;
  };

  virtual ~Reporter() = default;

  virtual std::span<std::string_view const> get_labels() const = 0;

  virtual void dispatch(Handler &, std::string_view const &label) const = 0;

  virtual void print(reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
  virtual void write(std::string_view const &path, reporter::OutputType = {}, std::string_view const &label = {}) const = 0;

  // host
  virtual void operator()(Event<Timer> const &) {}
  virtual void operator()(Event<Connected> const &) {}
  virtual void operator()(Event<Disconnected> const &) {}

  // control
  virtual void operator()(Event<DownloadBegin> const &) {}
  virtual void operator()(Event<DownloadEnd> const &) {}
  virtual void operator()(Event<Ready> const &) {}

  // config
  virtual void operator()(Event<GatewaySettings> const &) {}

  // stream
  virtual void operator()(Event<StreamStatus> const &) {}
  virtual void operator()(Event<ExternalLatency> const &) {}
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

  // order management
  virtual void operator()(Event<CancelAllOrdersAck> const &) {}
  virtual void operator()(Event<OrderAck> const &) {}
  virtual void operator()(Event<OrderUpdate> const &) {}
  virtual void operator()(Event<TradeUpdate> const &) {}

  // account management
  virtual void operator()(Event<PositionUpdate> const &) {}
  virtual void operator()(Event<FundsUpdate> const &) {}

  // client requests
  virtual void operator()(Event<CreateOrder> const &) {}
  virtual void operator()(Event<ModifyOrder> const &) {}
  virtual void operator()(Event<CancelOrder> const &) {}
  virtual void operator()(Event<CancelAllOrders> const &) {}

  // broadcast
  virtual void operator()(Event<CustomMetricsUpdate> const &) {}
  virtual void operator()(Event<CustomMatrixUpdate> const &) {}
};

}  // namespace algo
}  // namespace roq
