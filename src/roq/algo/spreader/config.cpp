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
      .regex = settings_.accounts,
  });
  // symbols
  handler(roq::client::Symbol{
      .regex = settings_.symbols,
      .exchange = settings_.exchange,
  });
  // currencies
  handler(roq::client::Symbol{
      .regex = settings_.currencies,
      .exchange = {},
  });
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
