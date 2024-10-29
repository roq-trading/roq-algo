/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/algo/reporter.hpp"

#include "roq/algo/reporter/type.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct ROQ_PUBLIC Factory final {
  static std::unique_ptr<Reporter> create(Type = {});
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
