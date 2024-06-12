/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <span>
#include <string_view>

#include "roq/service.hpp"

// note! the following are your implementations

#include "roq/algo/spreader/config.hpp"
#include "roq/algo/spreader/settings.hpp"
#include "roq/algo/spreader/strategy.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Application final : public Service {
  using Service::Service;  // inherit constructors

 protected:
  int main(args::Parser const &) override;

  void simulation(Settings const &, Config const &, std::span<std::string_view const> const &params);
  void trading(Settings const &, Config const &, std::span<std::string_view const> const &params);

 private:
  using value_type = Strategy;  // note!
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
