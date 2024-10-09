/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/algo/reporter/handler.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct ROQ_PUBLIC Summary final {
  struct Config final {
    std::chrono::nanoseconds frequency = {};
  };

  static std::unique_ptr<Handler> create();
  static std::unique_ptr<Handler> create(Config const &);
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
