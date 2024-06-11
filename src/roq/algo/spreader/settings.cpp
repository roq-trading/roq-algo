/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/settings.hpp"

namespace roq {
namespace algo {
namespace spreader {

Settings::Settings(roq::args::Parser const &args) : roq::client::flags::Settings{args}, flags::Flags{flags::Flags::create()}, test{flags::Test::create()} {
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
