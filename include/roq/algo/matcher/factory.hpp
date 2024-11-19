/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/algo/matcher.hpp"
#include "roq/algo/order_cache.hpp"

#include "roq/algo/matcher/config.hpp"
#include "roq/algo/matcher/type.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Factory final {
  static std::unique_ptr<Matcher> create(Type, Matcher::Dispatcher &, OrderCache &, Config const &);
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
