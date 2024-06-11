/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "simple/settings.hpp"

namespace simple {

Settings::Settings(roq::args::Parser const &args) : roq::client::flags::Settings{args}, flags::Flags{flags::Flags::create()}, test{flags::Test::create()} {
}

}  // namespace simple
