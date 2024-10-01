/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <string_view>

#include <fmt/core.h>

namespace roq {
namespace algo {

struct ROQ_PUBLIC Instrument final {
  uint8_t source = {};
  std::string_view exchange;
  std::string_view symbol;
  std::string_view account;
};

}  // namespace algo
}  // namespace roq

template <>
struct fmt::formatter<roq::algo::Instrument> {
  constexpr auto parse(format_parse_context &context) { return std::begin(context); }
  auto format(roq::algo::Instrument const &value, format_context &context) const {
    using namespace std::literals;
    return fmt::format_to(
        context.out(),
        R"({{)"
        R"(source={}, )"
        R"(exchange="{}", )"
        R"(symbol="{}", )"
        R"(account="{}")"
        R"(}})"sv,
        value.source,
        value.exchange,
        value.symbol,
        value.account);
  }
};
