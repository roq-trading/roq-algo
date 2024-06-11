/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/client/config.hpp"

#include "simple/settings.hpp"

namespace simple {

struct Config final : public roq::client::Config {
  explicit Config(Settings const &);

  Config(Config &&) = default;
  Config(Config const &) = delete;

 protected:
  void dispatch(Handler &) const override;

 private:
  Settings const &settings_;
};

}  // namespace simple
