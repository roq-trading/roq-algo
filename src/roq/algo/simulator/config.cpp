/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/simulator/config.hpp"

#include <toml++/toml.h>

#include "roq/logging.hpp"

#include "roq/exceptions.hpp"

#include "roq/utils/enum.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace simulator {

// === HELPERS ===

namespace {
void parse_position(auto &account, auto &node) {
  enum class Key {
    EXCHANGE,
    SYMBOL,
    LONG_POSITION,
    SHORT_POSITION,
  };
  std::string exchange, symbol;
  Position position;
  auto table = node.as_table();
  for (auto &[key, value] : *table) {
    auto key_2 = utils::parse_enum<Key>(key);
    switch (key_2) {
      case Key::EXCHANGE:
        exchange = value.template value<std::string>().value();
        break;
      case Key::SYMBOL:
        symbol = value.template value<std::string>().value();
        break;
      case Key::LONG_POSITION:
        position.long_position = value.template value<double>().value();
        break;
      case Key::SHORT_POSITION:
        position.short_position = value.template value<double>().value();
        break;
    }
  }
  if (std::empty(exchange) || std::empty(symbol)) {
    throw RuntimeError{"Both 'exchange' and 'symbol' required"sv};
  }
  if (std::isnan(position.long_position) && std::isnan(position.short_position)) {
    throw RuntimeError{"At least one of 'long_position' and 'short_position' required"sv};
  }
  account.exchanges[exchange].symbols[symbol].position = std::move(position);
}

void parse_accounts(auto &accounts, auto &node) {
  auto table = node.as_table();
  for (auto &[key, value] : *table) {
    auto &account = accounts[std::string{key}];
    auto arr = value.as_array();
    for (auto &node_2 : *arr) {
      parse_position(account, node_2);
    }
  }
}

void parse_sources(auto &sources, auto &node) {
  enum class Key {
    MARKET_DATA_LATENCY_MS,
    ORDER_MANAGEMENT_LATENCY_MS,
    ACCOUNTS,
  };
  auto arr = node.as_array();
  for (auto &node_2 : *arr) {
    auto table = node_2.as_table();
    roq::algo::simulator::Source source;
    for (auto &[key, value] : *table) {
      auto key_2 = utils::parse_enum<Key>(key);
      switch (key_2) {
        case Key::MARKET_DATA_LATENCY_MS: {
          auto tmp = value.template value<uint32_t>().value();
          source.market_data_latency = std::chrono::milliseconds{tmp};
          break;
        }
        case Key::ORDER_MANAGEMENT_LATENCY_MS: {
          auto tmp = value.template value<uint32_t>().value();
          source.order_management_latency = std::chrono::milliseconds{tmp};
          break;
        }
        case Key::ACCOUNTS:
          parse_accounts(source.accounts, value);
          break;
      }
    }
    sources.emplace_back(std::move(source));
  }
}

auto parse_helper(auto &root) {
  enum class Key {
    SOURCES,
  };
  Config result;
  auto table = root.as_table();
  for (auto &[key, value] : *table) {
    auto key_2 = utils::parse_enum<Key>(key);
    switch (key_2) {
      case Key::SOURCES:
        parse_sources(result.sources, value);
        break;
    }
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Config Config::parse_file(std::string_view const &path) {
  if (std::empty(path)) {
    return {};
  }
  log::info(R"(Parse simulator config file path="{}")"sv, path);
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

}  // namespace simulator
}  // namespace algo
}  // namespace roq
