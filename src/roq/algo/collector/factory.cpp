/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/collector/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/collector/summary.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace collector {

// === HELPERS ===

namespace {
struct None final : public client::Collector {};
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<client::Collector> Factory::create(Type type) {
  switch (type) {
    using enum Factory::Type;
    case NONE:
      return std::make_unique<None>();
    case SUMMARY:
      return std::make_unique<Summary>();
  }
  log::fatal("Unexpected: type={}"sv, magic_enum::enum_name(type));
}

}  // namespace collector
}  // namespace algo
}  // namespace roq
