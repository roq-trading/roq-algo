/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/reporter.hpp"

#include "roq/algo/reporter/factory.hpp"

using namespace std::literals;

namespace roq {
namespace algo {

// === IMPLEMENTATION ===

std::unique_ptr<Reporter> Reporter::create(reporter::Type type) {
  return reporter::Factory::create(type);
}

}  // namespace algo
}  // namespace roq
