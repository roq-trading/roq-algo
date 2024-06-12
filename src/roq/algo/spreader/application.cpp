/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/application.hpp"

#include <cassert>
#include <vector>

#include "roq/client.hpp"

#include "roq/algo/spreader/settings.hpp"

using namespace std::literals;
using namespace std::chrono_literals;  // NOLINT

namespace roq {
namespace algo {
namespace spreader {

// === CONSTANTS ===

namespace {
auto const SNAPSHOT_FREQUENCY = 1s;
auto const MATCHER = "simple"sv;  // note! filled when market is crossed
auto const MARKET_DATA_LATENCY = 1ms;
auto const ORDER_MANAGEMENT_LATENCY = 10ms;
}  // namespace

// === IMPLEMENTATION ===

int Application::main(args::Parser const &args) {
  auto params = args.params();
  if (std::empty(params)) {
    log::warn("You must provide at least one argument!"sv);
    log::warn("  For simulation: paths to event-logs (the .roq files created by gateways)"sv);
    log::warn("  For live trading: paths to unix sockets (the .sock files created by gateways)"sv);
    log::fatal("Unexpected"sv);
  }
  Settings settings{args};
  Config config{settings};
  if (settings.simulation) {
    simulation(settings, config, params);
  } else {
    trading(settings, config, params);
  }
  return EXIT_SUCCESS;
}

void Application::simulation(Settings const &settings, Config const &config, std::span<std::string_view const> const &params) {
  auto collector = client::detail::SimulationFactory::create_collector(SNAPSHOT_FREQUENCY);
  auto create_generator = [&params](auto source_id) { return client::detail::SimulationFactory::create_generator(params[source_id], source_id); };
  auto create_matcher = [](auto &dispatcher) { return client::detail::SimulationFactory::create_matcher(dispatcher, MATCHER); };
  auto factory = client::Simulator::Factory{
      .create_generator = create_generator,
      .create_matcher = create_matcher,
      .market_data_latency = MARKET_DATA_LATENCY,
      .order_management_latency = ORDER_MANAGEMENT_LATENCY,
  };
  client::Simulator{settings, config, factory, *collector}.dispatch<value_type>(settings);
}

void Application::trading(Settings const &settings, Config const &config, std::span<std::string_view const> const &params) {
  client::Trader{settings, config, params}.dispatch<value_type>(settings);
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
