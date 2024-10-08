/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/client/collector.hpp"

namespace roq {
namespace algo {
namespace collector {

struct Summary final : public client::Collector {
 protected:
  void operator()(Event<TradeUpdate> const &event) override;
};

}  // namespace collector
}  // namespace algo
}  // namespace roq
