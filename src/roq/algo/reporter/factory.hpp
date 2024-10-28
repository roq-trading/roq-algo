/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <memory>

#include "roq/algo/reporter.hpp"

#include "roq/algo/reporter/type.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct Factory final {
  static std::unique_ptr<Reporter> create(Type);
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
