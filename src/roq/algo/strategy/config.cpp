/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/strategy/config.hpp"

#include <toml++/toml.h>

#include "roq/logging.hpp"

#include "roq/exceptions.hpp"

#include "roq/utils/enum.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace strategy {

// === HELPERS ===

namespace {
void parse_legs(auto &legs, auto &node) {
  enum class Key {
    SOURCE,
    ACCOUNT,
    EXCHANGE,
    SYMBOL,
    POSITION_EFFECT,
    MARGIN_MODE,
    TIME_IN_FORCE,
  };
  auto arr = node.as_array();
  for (auto &node_2 : *arr) {
    auto table = node_2.as_table();
    roq::algo::Leg leg;
    for (auto &[key, value] : *table) {
      auto key_2 = utils::parse_enum<Key>(key);
      switch (key_2) {
        case Key::SOURCE:
          leg.source = value.template value<uint8_t>().value();
          break;
        case Key::ACCOUNT:
          leg.account = value.template value<std::string_view>().value();
          break;
        case Key::EXCHANGE:
          leg.exchange = value.template value<std::string_view>().value();
          break;
        case Key::SYMBOL:
          leg.symbol = value.template value<std::string_view>().value();
          break;
        case Key::POSITION_EFFECT: {
          auto tmp = value.template value<std::string_view>().value();
          leg.position_effect = utils::parse_enum<decltype(leg.position_effect)>(tmp);
          break;
        }
        case Key::MARGIN_MODE: {
          auto tmp = value.template value<std::string_view>().value();
          leg.margin_mode = utils::parse_enum<decltype(leg.margin_mode)>(tmp);
          break;
        }
        case Key::TIME_IN_FORCE: {
          auto tmp = value.template value<std::string_view>().value();
          leg.time_in_force = utils::parse_enum<decltype(leg.time_in_force)>(tmp);
          break;
        }
      }
    }
    legs.emplace_back(std::move(leg));
  }
}

auto parse_helper(auto &root) {
  enum class Key {
    LEGS,
    STRATEGY_ID,
  };
  Config result;
  auto table = root.as_table();
  for (auto &[key, value] : *table) {
    auto key_2 = utils::parse_enum<Key>(key);
    switch (key_2) {
      case Key::LEGS:
        parse_legs(result.legs, value);
        break;
      case Key::STRATEGY_ID:
        result.strategy_id = value.template value<uint32_t>().value();
        break;
    }
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Config Config::parse_file(std::string_view const &path) {
  log::info(R"(Parse strategy config file path="{}")"sv, path);
  try {
    auto root = toml::parse_file(path);
    return parse_helper(root);
  } catch (...) {
    log::error(R"(Failed to read or parse config file: path="{}")"sv, path);
    throw;
  }
}

Config Config::parse_text(std::string_view const &text) {
  auto root = toml::parse(text);
  return parse_helper(root);
}

}  // namespace strategy
}  // namespace algo
}  // namespace roq
