/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <string_view>

#include "roq/matcher.hpp"

namespace roq {
namespace algo {
namespace matcher {

struct ROQ_PUBLIC Factory final {
  enum class Type {
    SIMPLE,
  };

  static std::unique_ptr<Matcher> create(Type, Matcher::Dispatcher &, std::string_view const &exchange, std::string_view const &symbol);
};

}  // namespace matcher
}  // namespace algo
}  // namespace roq
