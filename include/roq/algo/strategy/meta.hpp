/* Copyright (c) 2017-2025, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <fmt/core.h>

#include <string_view>

#include "roq/variant_type.hpp"

namespace roq {
namespace algo {
namespace strategy {

struct ROQ_PUBLIC Meta final {
  std::string_view name;
  VariantType type = {};
  bool required = {};
  std::string_view description;
};

}  // namespace strategy
}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::strategy::Meta> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::strategy::Meta const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(name="{}", )"
        R"(type={}, )"
        R"(required={}, )"
        R"(description="{}")"
        R"(}})"sv,
        value.name,
        value.type,
        value.required,
        value.description);
  }
};
