/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/client/collector.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct Handler : public client::Collector {
  virtual void print() const = 0;
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
