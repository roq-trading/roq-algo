/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <span>
#include <string_view>

#include "roq/service.hpp"

// note! the following are your implementations

#include "simple/config.hpp"
#include "simple/settings.hpp"
#include "simple/strategy.hpp"

namespace simple {

struct Application final : public roq::Service {
  using Service::Service;  // inherit constructors

 protected:
  int main(roq::args::Parser const &) override;

  void simulation(Settings const &, Config const &, std::span<std::string_view const> const &params);
  void trading(Settings const &, Config const &, std::span<std::string_view const> const &params);

 private:
  using value_type = Strategy;  // note!
};

}  // namespace simple
