/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/config.hpp"

namespace roq {
namespace algo {
namespace spreader {

Config::Config(Settings const &settings) : settings_{settings} {
}

void Config::dispatch(Handler &handler) const {
  // settings
  handler(roq::client::Settings{
      .order_cancel_policy = roq::OrderCancelPolicy::BY_ACCOUNT,
      .order_management = {},
  });
  // accounts
  handler(roq::client::Account{
      .regex = settings_.account,
  });
  // symbols
  for (auto &item : settings_.symbols) {
    auto symbol = roq::client::Symbol{
        .regex = item,
        .exchange = settings_.exchange,
    };
    handler(symbol);
  }
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
