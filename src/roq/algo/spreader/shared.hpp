/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/client.hpp"

#include "roq/algo/spreader/settings.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Shared final {
  Shared(client::Dispatcher &dispatcher, Settings const &settings) : dispatcher{dispatcher}, settings{settings} {}

  inline uint64_t get_next_order_id() { return ++max_order_id; }

 public:
  client::Dispatcher &dispatcher;
  Settings const &settings;

  bool ready = {};
  uint64_t max_order_id = {};
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
