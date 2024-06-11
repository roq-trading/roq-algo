/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include <catch2/catch_test_macros.hpp>

#include "roq/algo/tools/simple.hpp"

using namespace roq;
using namespace roq::algo;
using namespace roq::algo::tools;

TEST_CASE("add", "[algo/spreader]") {
  CHECK(Simple::add(1, 2) == 3);
}
