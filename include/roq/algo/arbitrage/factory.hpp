/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <span>

#include "roq/algo/order_cache.hpp"

#include "roq/algo/strategy/config.hpp"
#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"
#include "roq/algo/strategy/meta.hpp"

#include "roq/algo/arbitrage/parameters.hpp"

namespace roq {
namespace algo {
namespace arbitrage {

struct ROQ_PUBLIC Factory final {
  static std::unique_ptr<strategy::Handler> create(strategy::Dispatcher &, OrderCache &, strategy::Config const &, Parameters const &);
  static std::unique_ptr<strategy::Handler> create(strategy::Dispatcher &, OrderCache &, strategy::Config const &, std::string_view const &parameters);

  static std::span<strategy::Meta const> get_meta();
};

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
