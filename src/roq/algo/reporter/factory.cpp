/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/reporter/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/reporter/summary.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace reporter {

// === HELPERS ===

namespace {
struct None final : public client::Reporter {
  virtual void print() const override {}
};
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<client::Reporter> Factory::create(Type type) {
  switch (type) {
    using enum Factory::Type;
    case NONE:
      return std::make_unique<None>();
    case SUMMARY:
      return std::make_unique<Summary>();
  }
  log::fatal("Unexpected: type={}"sv, magic_enum::enum_name(type));
}

}  // namespace reporter
}  // namespace algo
}  // namespace roq
