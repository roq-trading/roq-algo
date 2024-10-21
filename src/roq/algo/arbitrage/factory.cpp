/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/factory.hpp"

#include "roq/logging.hpp"

#include "roq/algo/arbitrage/simple.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {
template <typename T>
auto parse_enum(auto &value) {
  auto result = magic_enum::enum_cast<T>(value, magic_enum::case_insensitive);
  if (!result.has_value())
    log::fatal(R"(Unexpected: value="{}")"sv, value);
  return result.value();
}

enum class Key {
  MARKET_DATA_SOURCE,
  MAX_AGE,
  THRESHOLD,
  QUANTITY_0,
  MIN_POSITION_0,
  MAX_POSITION_0,
  PUBLISH_SOURCE,
};

auto parameters_from_string(auto &parameters) {
  Parameters result;
  auto callback_2 = [&](auto &key, auto &value) {
    assert(!std::empty(key));
    assert(!std::empty(value));
    auto key_2 = parse_enum<Key>(key);
    log::warn(R"(key={}, value="{}")"sv, magic_enum::enum_name(key_2), value);
    switch (key_2) {
      using enum Key;
      case MARKET_DATA_SOURCE:
        // result.market_data_source = MarketDataSource::TOP_OF_BOOK;  // XXX FIXME TODO parse value
        break;
      case MAX_AGE:
        result.max_age = 10s;  // XXX FIXME TODO parse value
        break;
      case THRESHOLD:
        result.threshold = 5.0;  // XXX FIXME TODO parse value
        break;
      case QUANTITY_0:
        result.quantity_0 = 1.0;  // XXX FIXME TODO parse value
        break;
      case MIN_POSITION_0:
        result.min_position_0 = -5.0;  // XXX FIXME TODO parse value
        break;
      case MAX_POSITION_0:
        result.max_position_0 = 5.0;  // XXX FIXME TODO parse value
        break;
      case PUBLISH_SOURCE:
        result.publish_source = 0;  // XXX FIXME TODO parse value
        break;
    }
  };
  auto callback_1 = [&](auto &key_value) {
    assert(!std::empty(key_value));
    auto pos = key_value.find('=');
    if (pos == key_value.npos)
      log::fatal(R"(Unexpected: key_value="{}" (parameters="{}")"sv, key_value, parameters);
    auto key = key_value.substr(0, pos);
    auto value = key_value.substr(pos + 1);
    callback_2(key, value);
  };
  auto tmp = parameters;
  while (true) {
    auto pos = tmp.find(';');
    if (pos == tmp.npos)
      break;
    auto tmp_1 = tmp.substr(0, pos);
    callback_1(tmp_1);
    tmp = tmp.substr(pos + 1);
  }
  if (!std::empty(tmp))
    callback_1(tmp);
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

std::unique_ptr<strategy::Handler> Factory::create(
    strategy::Dispatcher &dispatcher, OrderCache &order_cache, Config const &config, Parameters const &parameters) {
  return std::make_unique<Simple>(dispatcher, order_cache, config, parameters);
}

std::unique_ptr<strategy::Handler> Factory::create(
    strategy::Dispatcher &dispatcher, OrderCache &order_cache, Config const &config, std::string_view const &parameters) {
  auto parameters_2 = parameters_from_string(parameters);
  log::debug("parameters={}"sv, parameters_2);
  return create(dispatcher, order_cache, config, parameters_2);
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
