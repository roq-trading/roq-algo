/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {
template <typename R>
auto create_lookup(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = [&]() {
    size_t result = {};
    for (auto &instrument : config.instruments)
      result = std::max(result, instrument.source);
    return result + 1;
  }();
  result.resize(size);
  for (size_t i = 0; i < std::size(config.instruments); ++i) {
    auto &instrument = config.instruments[i];
    result[instrument.source][instrument.exchange][instrument.symbol] = i;
  }
  return result;
}

template <typename R>
auto create_state(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = std::size(config.instruments);
  if (size < 2)
    log::fatal("Unexpected: size={}"sv, size);
  result.resize(size);
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(strategy::Dispatcher &dispatcher, Config const &config, Cache &cache)
    : dispatcher_{dispatcher}, lookup_{create_state<decltype(lookup_)>(config)}, cache_{cache}, state_{create_state<decltype(state_)>(config)} {
}

void Simple::operator()(Event<Disconnected> const &event) {
  get_state(event, [](auto &state) { state = {}; });
}

void Simple::operator()(Event<DownloadBegin> const &) {
}

void Simple::operator()(Event<DownloadEnd> const &) {
}

void Simple::operator()(Event<Ready> const &event) {
  get_state(event, [](auto &state) { state.ready = true; });
}

void Simple::operator()(Event<ReferenceData> const &event) {
  auto &[message_info, reference_data] = event;
  auto &state = get_state(event);
  utils::update(state.tick_size, reference_data.tick_size);
}

void Simple::operator()(Event<MarketStatus> const &event) {
  auto &[message_info, market_status] = event;
  auto &state = get_state(event);
  utils::update(state.trading_status, market_status.trading_status);
}

void Simple::operator()(Event<TopOfBook> const &event) {
  auto &[message_info, top_of_book] = event;
  auto &state = get_state(event);
  utils::update(state.best, top_of_book.layer);
}

void Simple::operator()(Event<MarketByPriceUpdate> const &) {
}

void Simple::operator()(Event<MarketByOrderUpdate> const &) {
}

void Simple::operator()(Event<OrderAck> const &event, cache::Order const &) {
  auto &[message_info, order_ack] = event;
  auto &state = get_state(event);
}

void Simple::operator()(Event<OrderUpdate> const &event, cache::Order const &) {
  auto &[message_info, order_update] = event;
  auto &state = get_state(event);
}

void Simple::operator()(Event<PositionUpdate> const &event) {
  auto &[message_info, position_update] = event;
  auto &state = get_state(event);
}

// utils

template <typename Callback>
void Simple::get_state(MessageInfo const &message_info, Callback callback) {
  if (message_info.source < std::size(lookup_)) [[likely]] {
    auto &tmp_1 = lookup_[message_info.source];
    for (auto &[exchange, tmp_2] : tmp_1)
      for (auto &[symbol, index] : tmp_2)
        callback(state_[index]);
    return;
  }
  log::fatal(R"(Unexpected: source={})"sv, message_info.source);
}

template <typename T>
Simple::State &Simple::get_state(Event<T> const &event) {
  auto &[message_info, value] = event;
  if (message_info.source < std::size(lookup_)) [[likely]] {
    auto &tmp_1 = lookup_[message_info.source];
    auto iter_1 = tmp_1.find(value.exchange);
    if (iter_1 != std::end(tmp_1)) {
      auto &tmp_2 = (*iter_1).second;
      auto iter_2 = tmp_2.find(value.symbol);
      if (iter_2 != std::end(tmp_2))
        return state_[(*iter_2).second];
    }
  }
  log::fatal(R"(Unexpected: source={}, exchange="{}", symbol="{}")"sv, message_info.source, value.exchange, value.symbol);
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
