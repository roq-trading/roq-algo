/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/algo/order_cache.hpp"
#include "roq/algo/strategy.hpp"

#include "roq/algo/strategy/config.hpp"
#include "roq/algo/strategy/meta.hpp"
#include "roq/algo/strategy/type.hpp"

namespace roq {
namespace algo {
namespace strategy {

struct ROQ_PUBLIC Factory final {
  static std::unique_ptr<Strategy> create(Type, Strategy::Dispatcher &, OrderCache &, Config const &, std::string_view const &parameters);

  static std::span<Meta const> get_meta(Type);
};

}  // namespace strategy
}  // namespace algo
}  // namespace roq
