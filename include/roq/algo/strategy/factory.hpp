/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <span>
#include <string_view>

#include "roq/algo/order_cache.hpp"

#include "roq/algo/strategy/config.hpp"
#include "roq/algo/strategy/dispatcher.hpp"
#include "roq/algo/strategy/handler.hpp"
#include "roq/algo/strategy/meta.hpp"
#include "roq/algo/strategy/type.hpp"

namespace roq {
namespace algo {
namespace strategy {

struct ROQ_PUBLIC Factory final {
  static std::unique_ptr<strategy::Handler> create(Type, strategy::Dispatcher &, OrderCache &, Config const &, std::string_view const &parameters);

  static std::span<strategy::Meta const> get_meta(Type);
};

}  // namespace strategy
}  // namespace algo
}  // namespace roq
