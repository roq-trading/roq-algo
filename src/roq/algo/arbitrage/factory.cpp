/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/arbitrage/factory.hpp"

#include <fmt/format.h>

#include <magic_enum/magic_enum_format.hpp>

#include "roq/logging.hpp"

#include "roq/utils/enum.hpp"

#include "roq/utils/variant/parse.hpp"

#include "roq/utils/key_value/parser.hpp"

#include "roq/algo/arbitrage/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === CONSTANTS ===

namespace {
std::array<strategy::Meta, 7> const META{{
    {
        .name = "market_data_source"sv,
        .type = VariantType::ENUM,
        .required = false,
        .description = "Market data source"sv,
    },
    {
        .name = "max_age"sv,
        .type = VariantType::DATETIME,
        .required = false,
        .description = "Maximum market data age"sv,
    },
    {
        .name = "threshold"sv,
        .type = VariantType::DOUBLE,
        .required = false,
        .description = "Trade will be initiated if spread exceeds this value"sv,
    },
    {
        .name = "quantity_0"sv,
        .type = VariantType::DOUBLE,
        .required = false,
        .description = "Quantity of the first leg (index 0)"sv,
    },
    {
        .name = "min_position_0"sv,
        .type = VariantType::DOUBLE,
        .required = false,
        .description = "Minimum position of the first leg (index 0)"sv,
    },
    {
        .name = "max_position_0"sv,
        .type = VariantType::DOUBLE,
        .required = false,
        .description = "Maximum position of the first leg (index 0)"sv,
    },
    {
        .name = "publish_source"sv,
        .type = VariantType::UINT8,
        .required = false,
        .description = "Source (index) used for publishing custom metrics"sv,
    },
}};

auto const DEFAULT_MAX_AGE = 10s;
}  // namespace

// === HELPERS ===

namespace {
auto parameters_from_string(auto &parameters) {
  Parameters result;
  // parse
  auto callback = [&](auto &key, auto &value) {
    assert(!std::empty(key));
    assert(!std::empty(value));
    enum class Key {
      MARKET_DATA_SOURCE,
      MAX_AGE,
      THRESHOLD,
      QUANTITY_0,
      MIN_POSITION_0,
      MAX_POSITION_0,
      PUBLISH_SOURCE,
    };
    auto key_2 = utils::parse_enum<Key>(key);
    log::debug(R"(key={}, value="{}")"sv, key_2, value);
    switch (key_2) {
      case Key::MARKET_DATA_SOURCE:
        utils::variant::parse(result.market_data_source, value);
        break;
      case Key::MAX_AGE:
        result.max_age = DEFAULT_MAX_AGE;  // XXX FIXME TODO parse period
        break;
      case Key::THRESHOLD:
        utils::variant::parse(result.threshold, value);
        break;
      case Key::QUANTITY_0:
        utils::variant::parse(result.quantity_0, value);
        break;
      case Key::MIN_POSITION_0:
        utils::variant::parse(result.min_position_0, value);
        break;
      case Key::MAX_POSITION_0:
        utils::variant::parse(result.max_position_0, value);
        break;
      case Key::PUBLISH_SOURCE:
        utils::variant::parse(result.publish_source, value);
        break;
    }
  };
  utils::key_value::Parser::dispatch(parameters, callback);
  // validate
  // XXX FIXME TODO implement
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<Strategy> Factory::create(
    Strategy::Dispatcher &dispatcher, OrderCache &order_cache, strategy::Config const &config, Parameters const &parameters) {
  return std::make_unique<Simple>(dispatcher, order_cache, config, parameters);
}

std::unique_ptr<Strategy> Factory::create(
    Strategy::Dispatcher &dispatcher, OrderCache &order_cache, strategy::Config const &config, std::string_view const &parameters) {
  auto parameters_2 = parameters_from_string(parameters);
  log::debug("parameters={}"sv, parameters_2);
  return create(dispatcher, order_cache, config, parameters_2);
}

std::span<strategy::Meta const> Factory::get_meta() {
  return META;
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
