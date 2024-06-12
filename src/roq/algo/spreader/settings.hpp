/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/args/parser.hpp"

#include "roq/client/flags/settings.hpp"

#include "roq/algo/spreader/flags/flags.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Settings final : public client::flags::Settings, public flags::Flags {
  explicit Settings(args::Parser const &);
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
