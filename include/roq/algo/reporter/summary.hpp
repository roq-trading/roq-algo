/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/algo/market_data_source.hpp"
#include "roq/algo/reporter.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct ROQ_PUBLIC Summary final {
  struct Config final {
    MarketDataSource market_data_source;
    std::chrono::nanoseconds sample_frequency = {};
  };

  static std::unique_ptr<Reporter> create();
  static std::unique_ptr<Reporter> create(Config const &);
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
