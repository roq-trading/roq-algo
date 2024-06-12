/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

#include "roq/cache/market_by_price.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Instrument final {
  Instrument(std::string_view const &exchange, std::string_view const &symbol);

  void clear();

  void operator()(Event<ReferenceData> const &);
  void operator()(Event<MarketStatus> const &);
  void operator()(Event<MarketByPriceUpdate> const &);

  void operator()(Event<OrderAck> const &);
  void operator()(Event<OrderUpdate> const &);
  void operator()(Event<TradeUpdate> const &);
  void operator()(Event<PositionUpdate> const &);

 private:
  std::unique_ptr<cache::MarketByPrice> market_by_price_;
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
