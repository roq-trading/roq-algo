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

Instrument::Instrument(Shared &shared, std::string_view const &exchange, std::string_view const &symbol, Side side, double total_quantity, double weight)
    : shared_{shared}, symbol_{symbol}, side_{side}, total_quantity_{total_quantity}, weight_{weight},
      market_by_price_{create_market_by_price(exchange, symbol)} {
  assert(side_ != Side{});
  assert(!std::isnan(total_quantity_));
}

void Instrument::clear() {
  (*market_by_price_).clear();
  // reference data
  //   note! keeping all reference data
  // market data
  top_of_book_ = {};
  impact_price_ = std::numeric_limits<double>::quiet_NaN();
  target_price_ = std::numeric_limits<double>::quiet_NaN();
  market_data_ready_ = false;
  // order management
  //   XXX TODO
  // all
  ready_ = false;
}

bool Instrument::operator()(Event<ReferenceData> const &event) {
  (*market_by_price_)(event.value);
  auto &reference_data = event.value;
  auto updated = false;
  if (utils::update(min_trade_vol_, reference_data.min_trade_vol)) {
    log::info("[{}:{}] min_trade_vol={}"sv, symbol_, side_, min_trade_vol_);
    updated = true;
  }
  if (utils::update(tick_size_, reference_data.tick_size)) {
    log::info("[{}:{}] tick_size={}"sv, symbol_, side_, tick_size_);
    updated = true;
  }
  if (!updated)
    return false;
  return update_reference_data();
}

bool Instrument::operator()(Event<MarketStatus> const &) {
  // XXX TODO
  return false;
}

bool Instrument::operator()(Event<MarketByPriceUpdate> const &event) {
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
  log::info("[{}:{}] impact_price={}"sv, symbol_, side_, impact_price_);
  update_market_data();
  return true;
}

void Instrument::operator()(Event<OrderAck> const &event) {
  auto &order_ack = event.value;
  // note! fail hard so we can resolve any issues
  if (utils::has_request_failed(order_ack.request_status))
    log::fatal("order_ack={}"sv, order_ack);
  if (!utils::has_request_completed(order_ack.request_status))
    return;
  switch (order_ack.request_type) {
    using enum RequestType;
    case UNDEFINED:
      assert(false);
      break;
    case CREATE_ORDER:
      assert(state_ == State::CREATING);
      (*this)(State::READY);
      assert(std::isnan(order_price_));
      break;
    case MODIFY_ORDER:
      assert(state_ == State::MODIFYING);
      (*this)(State::READY);
      break;
    case CANCEL_ORDER:
      assert(false);
      break;
  }
  assert(!std::isnan(request_price_));
  request_price_ = std::numeric_limits<double>::quiet_NaN();
  if (utils::update(order_price_, order_ack.price))
    log::info("[{}:{}] order_price={} (version={})"sv, symbol_, side_, order_price_, order_ack.version);
  refresh();
}

void Instrument::operator()(Event<OrderUpdate> const &) {
}

void Instrument::operator()(Event<TradeUpdate> const &) {
}

void Instrument::operator()(Event<PositionUpdate> const &) {
}

// state change
bool Instrument::update_reference_data() {
  auto reference_data_ready = [&]() {
    if (std::isnan(min_trade_vol_) || utils::is_zero(min_trade_vol_) || std::isnan(tick_size_) || utils::is_zero(tick_size_))
      return false;
    return true;
  }();
  if (!utils::update(reference_data_ready_, reference_data_ready))
    return false;
  log::info("[{}:{}] reference_data_ready={}"sv, symbol_, side_, reference_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("[{}:{}] ready={}"sv, symbol_, side_, ready_);
  return true;
}

// state change
bool Instrument::update_market_data() {
  // XXX TODO check market status
  auto market_data_ready = !std::isnan(impact_price_);
  if (!utils::update(market_data_ready_, market_data_ready))
    return false;
  log::info("[{}:{}] market_data_ready={}"sv, symbol_, side_, market_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("[{}:{}] ready={}"sv, symbol_, side_, ready_);
  return true;
}

// market data change
void Instrument::update(double residual) {
  // note!
  //   the residual computed from impact price of all insruments in portfolio
  //   because it's a linear model, we can easily compute this instrument's target price from its impact price
  auto target_spread = shared_.settings.params[0];
  auto delta = (target_spread - residual) / weight_;
  auto raw_target_price = impact_price_ + delta;
  // note! must be rounded away to at least achieve the target spread
  auto target_price = market::round_away(side_, raw_target_price, tick_size_);
  if (utils::update(target_price_, target_price)) {
    log::info("[{}:{}] target_price={}"sv, symbol_, side_, target_price_);
    DEBUG_print();
    refresh();
  }
}

// target price change
void Instrument::refresh() {
  assert(!std::isnan(target_price_));
  if (state_ != State::READY)
    return;
  assert(std::isnan(request_price_));
  if (!order_id_) {
    assert(std::isnan(order_price_));
    order_id_ = shared_.get_next_order_id();
    auto create_order = CreateOrder{
        .account = shared_.settings.account,
        .order_id = order_id_,
        .exchange = shared_.settings.exchange,
        .symbol = symbol_,
        .side = side_,
        .position_effect = {},
        .margin_mode = {},
        .max_show_quantity = std::numeric_limits<double>::quiet_NaN(),
        .order_type = OrderType::LIMIT,
        .time_in_force = TimeInForce::GTC,
        .execution_instructions = {},
        .request_template = {},
        .quantity = total_quantity_,
        .price = target_price_,
        .stop_price = std::numeric_limits<double>::quiet_NaN(),
        .routing_id = {},
        .strategy_id = shared_.settings.strategy_id,
    };
    log::info("[{}:{}] create_order={}"sv, symbol_, side_, create_order);
    shared_.dispatcher.send(create_order, 0u);
    state_ = State::CREATING;
  } else {
    if (utils::compare(order_price_, target_price_) == 0) {
      log::info("SAME order_price={}, target_price={}"sv, order_price_, target_price_);
      return;
    }
    auto modify_order = ModifyOrder{
        .account = shared_.settings.account,
        .order_id = order_id_,
        .request_template = {},
        .quantity = std::numeric_limits<double>::quiet_NaN(),
        .price = target_price_,
        .routing_id = {},
        .version = {},
        .conditional_on_version = {},
    };
    log::info("[{}:{}] modify_order={}"sv, symbol_, side_, modify_order);
    shared_.dispatcher.send(modify_order, 0u);
    state_ = State::MODIFYING;
  }
  request_price_ = target_price_;
}

void Instrument::operator()(State state) {
  if (utils::update(state_, state))
    log::info("[{}:{}] state={}"sv, symbol_, side_, magic_enum::enum_name(state_));
}

void Instrument::DEBUG_print() {
  log::debug("[{}:{}] target_price={}, impact_price={}, top_of_book={}"sv, symbol_, side_, target_price_, impact_price_, top_of_book_);
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
