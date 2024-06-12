/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/instrument.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

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

// note! rounding happens later (wait for reference data / min-trade-vol)

Instrument::Instrument(std::string_view const &exchange, std::string_view const &symbol, Side side, double total_quantity)
    : side_{side}, total_quantity_{total_quantity}, market_by_price_{create_market_by_price(exchange, symbol)} {
  assert(side_ != Side{});
  assert(!std::isnan(total_quantity_));
}

void Instrument::clear() {
  (*market_by_price_).clear();
  // reference data
  // note! keeping reference data (shouldn't change)
  // market data
  impact_price_ = std::numeric_limits<double>::quiet_NaN();
  market_data_ready_ = false;
  // all
  ready_ = false;
}

bool Instrument::operator()(Event<ReferenceData> const &event) {
  log::info<1>("event={}"sv, event);
  (*market_by_price_)(event.value);
  auto &reference_data = event.value;
  auto updated = false;
  if (utils::update(min_trade_vol_, reference_data.min_trade_vol)) {
    log::info("DEBUG min_trade_vol={}"sv, min_trade_vol_);
    updated = true;
  }
  if (utils::update(tick_size_, reference_data.tick_size)) {
    log::info("DEBUG tick_size={}"sv, tick_size_);
    updated = true;
  }
  if (updated)
    return update_reference_data();
  return false;
}

bool Instrument::operator()(Event<MarketStatus> const &event) {
  log::info<1>("event={}"sv, event);
  return false;
}

bool Instrument::operator()(Event<MarketByPriceUpdate> const &event) {
  log::info<3>("event={}"sv, event);
  (*market_by_price_)(event.value);
  auto impact_price = [&]() {
    auto layer = (*market_by_price_).compute_impact_price(total_quantity_);
    // log::info("DEBUG impact_price({}) ==> layer={}"sv, total_quantity_, layer);
    switch (side_) {
      using enum Side;
      case UNDEFINED:
        assert(false);
        break;
      case BUY:
        return layer.ask_price;
      case SELL:
        return layer.bid_price;
    }
    return std::numeric_limits<double>::quiet_NaN();
  }();
  auto updated = [&]() {
    if (std::isnan(impact_price) && !std::isnan(impact_price_)) {
      impact_price_ = std::numeric_limits<double>::quiet_NaN();
      return true;
    } else {
      return utils::update(impact_price_, impact_price);
    }
  }();
  if (updated) {
    log::info("DEBUG impact_price={}"sv, impact_price_);
    return update_market_data();
  }
  return false;
}

void Instrument::operator()(Event<OrderAck> const &) {
}

void Instrument::operator()(Event<OrderUpdate> const &) {
}

void Instrument::operator()(Event<TradeUpdate> const &) {
}

void Instrument::operator()(Event<PositionUpdate> const &) {
}

bool Instrument::update_reference_data() {
  auto reference_data_ready = [&]() {
    if (std::isnan(min_trade_vol_) || utils::is_zero(min_trade_vol_) || std::isnan(tick_size_) || utils::is_zero(tick_size_))
      return false;
    return true;
  }();
  if (!utils::update(reference_data_ready_, reference_data_ready))
    return false;
  log::info("DEBUG reference_data_ready={}"sv, reference_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("DEBUG ready={}"sv, ready_);
  return true;
}

bool Instrument::update_market_data() {
  auto market_data_ready = true;  // XXX TODO
  if (!utils::update(market_data_ready_, market_data_ready))
    return false;
  log::info("DEBUG market_data_ready={}"sv, market_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("DEBUG ready={}"sv, ready_);
  return true;
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
