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
struct None final : public Reporter {
  virtual void print(OutputType, std::string_view const &) const override {}
  virtual void write(std::string_view const &, OutputType, std::string_view const &) const override {}
};
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<Reporter> Factory::create(Type type) {
  switch (type) {
    using enum Type;
    case NONE:
      return std::make_unique<None>();
    case SUMMARY:
      return Summary::create();
  }
  log::fatal("Unexpected: type={}"sv, magic_enum::enum_name(type));
}

}  // namespace reporter
}  // namespace algo
}  // namespace roq
