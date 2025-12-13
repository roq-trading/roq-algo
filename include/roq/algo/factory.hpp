/* Copyright (c) 2017-2026, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <string_view>

#include "roq/algo/order_cache.hpp"

#include "roq/algo/strategy.hpp"
#include "roq/algo/strategy/config.hpp"

#include "roq/algo/reporter.hpp"

#include "roq/algo/matcher.hpp"
#include "roq/algo/matcher/config.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Factory {
  virtual std::unique_ptr<algo::Strategy> create_strategy(algo::Strategy::Dispatcher &, algo::OrderCache &, algo::strategy::Config const &) const = 0;
  virtual std::unique_ptr<algo::Reporter> create_reporter() const = 0;
  virtual std::unique_ptr<algo::Matcher> create_matcher(algo::Matcher::Dispatcher &, algo::OrderCache &, algo::matcher::Config const &) const = 0;
};

}  // namespace algo
}  // namespace roq
