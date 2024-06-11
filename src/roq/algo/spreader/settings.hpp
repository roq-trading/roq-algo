/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/args/parser.hpp"

#include "roq/client/flags/settings.hpp"

#include "roq/algo/spreader/flags/flags.hpp"
#include "roq/algo/spreader/flags/test.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Settings final : public roq::client::flags::Settings, public flags::Flags {
  explicit Settings(roq::args::Parser const &);

  flags::Test const test;  // note! flags/test.json uses "prefix"
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
