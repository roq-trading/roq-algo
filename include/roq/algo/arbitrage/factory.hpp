/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/algo/cache.hpp"

#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"

#include "roq/algo/arbitrage/config.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct ROQ_PUBLIC Factory final {
  static std::unique_ptr<strategy::Handler> create(strategy::Dispatcher &, Config const &, Cache &);
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
