/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/client/config.hpp"

#include "roq/algo/spreader/settings.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Config final : public client::Config {
  explicit Config(Settings const &);

  Config(Config &&) = default;
  Config(Config const &) = delete;

 protected:
  void dispatch(Handler &) const override;

 private:
  Settings const &settings_;
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
