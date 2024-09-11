/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <string_view>

#include "roq/algo/matcher/cache.hpp"
#include "roq/algo/matcher/config.hpp"
#include "roq/algo/matcher/dispatcher.hpp"
#include "roq/algo/matcher/handler.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Factory final {
  enum class Type {
    SIMPLE,
  };

  static std::unique_ptr<Handler> create(Type, Dispatcher &, Cache &, std::string_view const &exchange, std::string_view const &symbol, Config const &);
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
