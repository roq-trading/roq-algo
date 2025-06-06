/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/reporter/factory.hpp"

#include <fmt/format.h>

#include <magic_enum/magic_enum_format.hpp>

#include "roq/logging.hpp"

#include "roq/algo/reporter/summary.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace reporter {

// === HELPERS ===

namespace {
struct None final : public Reporter {
  std::span<std::string_view const> get_labels() const override { return {}; }
  void dispatch(Handler &, [[maybe_unused]] std::string_view const &label) const override { throw RuntimeError{"not supported"sv}; }
  void print(OutputType, std::string_view const &) const override {}
  void write(std::string_view const &, OutputType, std::string_view const &) const override {}
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
  log::fatal("Unexpected: type={}"sv, type);
}

}  // namespace reporter
}  // namespace algo
}  // namespace roq
