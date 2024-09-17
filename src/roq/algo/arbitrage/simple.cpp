/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(strategy::Dispatcher &dispatcher, Config const &config, Cache &cache) : dispatcher_{dispatcher}, config_{config}, cache_{cache} {
}

void Simple::operator()(Event<Connected> const &) {
}

void Simple::operator()(Event<Disconnected> const &event) {
  auto &state = get_state(event);
  state = {};
}

void Simple::operator()(Event<DownloadBegin> const &) {
}

void Simple::operator()(Event<DownloadEnd> const &) {
}

void Simple::operator()(Event<Ready> const &event) {
  auto &state = get_state(event);
  state.ready = true;
}

void Simple::operator()(Event<ReferenceData> const &event) {
  auto &[message_info, reference_data] = event;
  auto &state = get_state(message_info);
  utils::update(state.tick_size, reference_data.tick_size);
}

void Simple::operator()(Event<MarketStatus> const &event) {
  auto &[message_info, market_status] = event;
  auto &state = get_state(message_info);
  utils::update(state.trading_status, market_status.trading_status);
}

void Simple::operator()(Event<TopOfBook> const &event) {
  auto &[message_info, top_of_book] = event;
  auto &state = get_state(message_info);
  utils::update(state.best, top_of_book.layer);
}

void Simple::operator()(Event<OrderAck> const &, cache::Order const &) {
}

void Simple::operator()(Event<OrderUpdate> const &, cache::Order const &) {
}

void Simple::operator()(Event<TradeUpdate> const &, cache::Order const &) {
}

Simple::State &Simple::get_state(MessageInfo const &message_info) {
  if (message_info.source < 2)
    return state_[message_info.source];
  log::fatal("Unexpected: message_info={}"sv, message_info);
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
