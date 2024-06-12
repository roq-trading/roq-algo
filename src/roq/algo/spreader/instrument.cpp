/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/instrument.hpp"

#include "roq/logging.hpp"

#include "roq/market/mbp/factory.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace spreader {

// === HELPERS ===

namespace {
auto create_market_by_price(auto &exchange, auto &symbol) {
  return market::mbp::Factory::create(exchange, symbol);
}
}  // namespace

// === IMPLEMENTATION ===

Instrument::Instrument(std::string_view const &exchange, std::string_view const &symbol) : market_by_price_{create_market_by_price(exchange, symbol)} {
}

void Instrument::clear() {
  (*market_by_price_).clear();
}

void Instrument::operator()(Event<ReferenceData> const &event) {
  log::info("event={}"sv, event);
  (*market_by_price_)(event.value);
}

void Instrument::operator()(Event<MarketStatus> const &event) {
  log::info("event={}"sv, event);
}

void Instrument::operator()(Event<MarketByPriceUpdate> const &event) {
  log::info("event={}"sv, event);
  (*market_by_price_)(event.value);
}

void Instrument::operator()(Event<OrderAck> const &) {
}

void Instrument::operator()(Event<OrderUpdate> const &) {
}

void Instrument::operator()(Event<TradeUpdate> const &) {
}

void Instrument::operator()(Event<PositionUpdate> const &) {
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
