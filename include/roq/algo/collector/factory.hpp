/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>

#include "roq/client/collector.hpp"

namespace roq {
namespace algo {
namespace collector {

struct ROQ_PUBLIC Factory final {
  enum class Type {
    NONE,
    SUMMARY,
  };

  static std::unique_ptr<client::Collector> create(Type);
};

}  // namespace collector
}  // namespace algo
}  // namespace roq
