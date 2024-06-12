/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/settings.hpp"

namespace roq {
namespace algo {
namespace spreader {

// === IMPLEMENTATION ===

Settings::Settings(args::Parser const &args) : client::flags::Settings{args}, flags::Flags{flags::Flags::create()} {
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
