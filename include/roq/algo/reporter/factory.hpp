/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/client.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct ROQ_PUBLIC Factory final {
  enum class Type {
    NONE,
    SUMMARY,
  };

  static std::unique_ptr<client::Simulator2::Reporter> create(Type);
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
