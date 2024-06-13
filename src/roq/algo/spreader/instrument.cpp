/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/instrument.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/market/utils.hpp"

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

Instrument::Instrument(std::string_view const &exchange, std::string_view const &symbol, Side side, double total_quantity, double weight, double target_spread)
    : symbol_{symbol}, side_{side}, total_quantity_{total_quantity}, weight_{weight}, target_spread_{target_spread},
      market_by_price_{create_market_by_price(exchange, symbol)} {
  assert(side_ != Side{});
  assert(!std::isnan(total_quantity_));
}

void Instrument::clear() {
  (*market_by_price_).clear();
  // reference data
  //   note! keeping reference data (shouldn't change)
  // market data
  top_of_book_ = {};
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
    log::info("DEBUG [{}] min_trade_vol={}"sv, symbol_, min_trade_vol_);
    updated = true;
  }
  if (utils::update(tick_size_, reference_data.tick_size)) {
    log::info("DEBUG [{}] tick_size={}"sv, symbol_, tick_size_);
    updated = true;
  }
  if (!updated)
    return false;
  return update_reference_data();
}

bool Instrument::operator()(Event<MarketStatus> const &event) {
  log::info<1>("event={}"sv, event);
  return false;
}

bool Instrument::operator()(Event<MarketByPriceUpdate> const &event) {
  log::info<3>("event={}"sv, event);
  (*market_by_price_)(event.value);
  (*market_by_price_).extract({&top_of_book_, 1});
  auto impact_price = [&]() {
    auto layer = (*market_by_price_).compute_impact_price(total_quantity_);
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
  if (!updated)
    return false;
  log::info("DEBUG [{}] impact_price={}"sv, symbol_, impact_price_);
  update_market_data();
  return true;
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
  log::info("DEBUG [{}] reference_data_ready={}"sv, symbol_, reference_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("DEBUG [{}] ready={}"sv, symbol_, ready_);
  return true;
}

bool Instrument::update_market_data() {
  // XXX TODO check market status
  auto market_data_ready = !std::isnan(impact_price_);
  if (!utils::update(market_data_ready_, market_data_ready))
    return false;
  log::info("DEBUG [{}] market_data_ready={}"sv, symbol_, market_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("DEBUG [{}] ready={}"sv, symbol_, ready_);
  return true;
}

double Instrument::value() const {
  return weight_ * impact_price_;
}

void Instrument::update(double residual) {
  auto target_price = impact_price_ + (target_spread_ - residual) / weight_;
  auto target_price_2 = market::round_conservative(side_, target_price, tick_size_);
  log::info(
      "DEBUG [{}] side={}, target_price={}({}/{}) (top_of_book={}/{})"sv,
      symbol_,
      side_,
      target_price_2,
      target_price,
      tick_size_,
      top_of_book_.bid_price,
      top_of_book_.ask_price);
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq