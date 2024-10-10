/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/instrument.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === IMPLEMENTATION ===

Instrument::Instrument(algo::Instrument const &item, MarketDataSource market_data_source)
    : source{item.source}, exchange{item.exchange}, symbol{item.symbol}, account{item.account}, market_data_{exchange, symbol, market_data_source} {
}

bool Instrument::is_ready(MessageInfo const &message_info, std::chrono::nanoseconds max_age) const {
  if (order_state != OrderState::IDLE)
    return false;
  assert(order_id == 0);
  // XXX FIXME TODO check rate-limit throttling
  auto has_liquidity = [](auto price, auto quantity) { return !std::isnan(price) && !std::isnan(quantity) && quantity > 0.0; };
  auto &top_of_book = market_data_.top_of_book();
  return is_market_active(message_info, max_age) && has_liquidity(top_of_book.bid_price, top_of_book.bid_quantity) &&
         has_liquidity(top_of_book.ask_price, top_of_book.ask_quantity);
}

void Instrument::reset() {
  order_state = {};
  order_id = {};
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
