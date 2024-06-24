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
auto compute_threshold_quantity(auto &settings, auto total_quantity) {
  auto multiplier = std::isnan(settings.threshold_quantity_multiplier) ? 1.0 : settings.threshold_quantity_multiplier;
  return total_quantity * multiplier;
}

auto create_market_by_price(auto &exchange, auto &symbol) {
  return market::mbp::Factory::create(exchange, symbol);
}
}  // namespace

// === IMPLEMENTATION ===

Instrument::Instrument(Shared &shared, std::string_view const &exchange, std::string_view const &symbol, Side side, double total_quantity, double weight)
    : shared_{shared}, symbol_{symbol}, side_{side}, total_quantity_{total_quantity}, weight_{weight},
      threshold_quantity_{compute_threshold_quantity(shared_.settings, total_quantity_)}, market_by_price_{create_market_by_price(exchange, symbol)},
      target_quantity_{total_quantity_} {
  assert(side_ != Side{});
  assert(utils::compare(total_quantity_, 0.0) > 0);
  assert(utils::compare(threshold_quantity_, 0.0) > 0);
}

void Instrument::clear() {
  (*market_by_price_).clear();
  // reference data
  //   note! keeping all reference data
  // market data
  trading_status_ = {};
  top_of_book_ = {};
  impact_price_ = NaN;
  target_price_ = NaN;
  market_data_ready_ = false;
  // order management
  //   XXX TODO
  // all
  ready_ = false;
}

void Instrument::operator()(Event<Timer> const &event) {
  auto now = event.value.now;
  if (next_refresh_.count() == 0 || now < next_refresh_)
    return;
  refresh(now);
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

bool Instrument::operator()(Event<MarketStatus> const &event) {
  if (utils::update(trading_status_, event.value.trading_status))
    return update_market_data();
  return false;
}

bool Instrument::operator()(Event<MarketByPriceUpdate> const &event) {
  (*market_by_price_)(event.value);
  // note! top of book only used for reference
  (*market_by_price_).extract({&top_of_book_, 1});
  // note!
  //   impact price is the worst expected price needed to aggress our quantity against resting orders
  //   in a best case scenario (nothing else changing) we would achieve a volume weighted average price (which would be better for us)
  auto impact_price = [&]() {
    auto layer = (*market_by_price_).compute_impact_price(threshold_quantity_, shared_.settings.threshold_number_of_orders);
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
    return NaN;  // note! we ensure the linear model evaluates to NaN when this price becomes "unavailable"
  }();
  auto updated = [&]() {
    if (std::isnan(impact_price) && !std::isnan(impact_price_)) {
      impact_price_ = NaN;
      return true;
    } else {
      return utils::update(impact_price_, impact_price);
    }
  }();
  if (!updated)
    return false;
  auto best_price = [&]() {
    switch (side_) {
      using enum Side;
      case UNDEFINED:
        assert(false);
        break;
      case BUY:
        return top_of_book_.ask_price;
      case SELL:
        return top_of_book_.bid_price;
    }
    return NaN;
  }();
  log::info("[{}:{}] impact_price={} (best_price={})"sv, symbol_, side_, impact_price_, best_price);
  update_market_data();
  return true;
}

bool Instrument::operator()(Event<OrderAck> const &event) {
  auto &order_ack = event.value;
  if (utils::has_request_failed(order_ack.request_status))
    log::fatal("order_ack={}"sv, order_ack);  // note! fail hard so we can resolve any issues
  if (utils::has_request_completed(order_ack.request_status)) {
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
    request_price_ = NaN;
    if (utils::update(order_price_, order_ack.price))
      log::info("[{}:{}] order_price={} (version={})"sv, symbol_, side_, order_price_, order_ack.version);
    auto now = clock::get_system();
    refresh(now);
  }
  return false;
}

bool Instrument::operator()(Event<OrderUpdate> const &event) {
  auto &order_update = event.value;
  return utils::update_max(traded_quantity_, order_update.traded_quantity);
}

bool Instrument::operator()(Event<TradeUpdate> const &) {
  return false;
}

bool Instrument::operator()(Event<PositionUpdate> const &) {
  return false;
}

// reference data have changed
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

// our market data have changed
bool Instrument::update_market_data() {
  // XXX TODO check market status
  auto market_data_ready = trading_status_ == TradingStatus::OPEN && !std::isnan(impact_price_);
  if (!utils::update(market_data_ready_, market_data_ready))
    return false;
  log::info("[{}:{}] market_data_ready={}"sv, symbol_, side_, market_data_ready_);
  auto ready = reference_data_ready_ && market_data_ready_;
  if (utils::update(ready_, ready))
    log::info("[{}:{}] ready={}"sv, symbol_, side_, ready_);
  return true;
}

// any market data have changed
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
    log::info(
        "[{}:{}] RESIDUAL={}, TARGET_SPREAD={}, DELTA={}, RAW_TARGET_PRICE={}, IMPACT_PRICE={}"sv,
        symbol_,
        side_,
        residual,
        target_spread,
        delta,
        raw_target_price,
        impact_price_);
    DEBUG_print();
    auto now = clock::get_system();
    refresh(now);
  }
}

// target price has changed
void Instrument::refresh(std::chrono::nanoseconds now) {
  if (!ready())
    return;
  assert(!std::isnan(target_price_));
  if (state_ != State::READY)
    return;
  assert(std::isnan(request_price_));
  if (!order_id_) {
    create_order();
  } else {
    auto result = utils::compare(order_price_, target_price_);
    if (result == 0) {
      log::info("[{}:{}] *** UNCHANGED *** (order_price={}, target_price={})"sv, symbol_, side_, order_price_, target_price_);
      return;
    }
    // note!
    //   trying to reduce the number of order modifications
    //   we can wait a little if the spread widens in our favour
    auto can_wait = [&]() {
      switch (side_) {
        using enum Side;
        case UNDEFINED:
          assert(false);
          break;
        case BUY:
          return result < 0;
        case SELL:
          return result > 0;
      }
      return false;
    }();
    // XXX TODO also check distance to best
    if (can_wait) {
      if (next_refresh_.count() == 0)
        next_refresh_ = now + shared_.settings.delay;
      if (now < next_refresh_) {
        log::info("[{}:{}] *** WAIT *** (order_price={}, target_price={})"sv, symbol_, side_, order_price_, target_price_);
        return;
      }
    }
    modify_price();
  }
  request_price_ = target_price_;
}

void Instrument::create_order() {
  assert(order_id_);
  assert(std::isnan(order_price_));
  assert(!std::isnan(target_price_));
  order_id_ = shared_.get_next_order_id();
  auto create_order = CreateOrder{
      .account = shared_.settings.account,
      .order_id = order_id_,
      .exchange = shared_.settings.exchange,
      .symbol = symbol_,
      .side = side_,
      .position_effect = {},
      .margin_mode = {},
      .max_show_quantity = NaN,
      .order_type = OrderType::LIMIT,
      .time_in_force = TimeInForce::GTC,
      .execution_instructions = {},
      .request_template = {},
      .quantity = target_quantity_,
      .price = target_price_,
      .stop_price = NaN,
      .routing_id = {},
      .strategy_id = shared_.settings.strategy_id,
  };
  log::info("[{}:{}] *** CREATE *** create_order={}"sv, symbol_, side_, create_order);
  try {
    shared_.dispatcher.send(create_order, 0u);
    state_ = State::CREATING;
  } catch (NotConnected &e) {
    log::info("[{}:{}] {}"sv, symbol_, side_, e);
    // note! we expect this to be followed by the Disconnected event
    // XXX TODO review if we need to do something here
  }
}

void Instrument::modify_price() {
  assert(!order_id_);
  assert(!std::isnan(order_price_));
  assert(!std::isnan(target_price_));
  assert(utils::compare(order_price_, target_price_) != 0);
  auto modify_order = ModifyOrder{
      .account = shared_.settings.account,
      .order_id = order_id_,
      .request_template = {},
      .quantity = NaN,
      .price = target_price_,
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
  };
  log::info("[{}:{}] *** MODIFY *** modify_order={}"sv, symbol_, side_, modify_order);
  try {
    shared_.dispatcher.send(modify_order, 0u);
    state_ = State::MODIFYING;
    next_refresh_ = {};
  } catch (NotConnected &e) {
    log::info("[{}:{}] {}"sv, symbol_, side_, e);
    // note! we expect this to be followed by the Disconnected event
    // XXX TODO review if we need to do something here
  }
}

void Instrument::modify_quantity() {
  assert(!order_id_);
  assert(!std::isnan(order_price_));
  assert(!std::isnan(target_price_));
  assert(utils::compare(order_price_, target_price_) != 0);
  auto modify_order = ModifyOrder{
      .account = shared_.settings.account,
      .order_id = order_id_,
      .request_template = {},
      .quantity = target_quantity_,
      .price = NaN,
      .routing_id = {},
      .version = {},
      .conditional_on_version = {},
  };
  log::info("[{}:{}] *** MODIFY *** modify_order={}"sv, symbol_, side_, modify_order);
  try {
    shared_.dispatcher.send(modify_order, 0u);
    state_ = State::MODIFYING;
    next_refresh_ = {};
  } catch (NotConnected &e) {
    log::info("[{}:{}] {}"sv, symbol_, side_, e);
    // note! we expect this to be followed by the Disconnected event
    // XXX TODO review if we need to do something here
  }
}

void Instrument::operator()(State state) {
  if (utils::update(state_, state))
    log::info("[{}:{}] state={}"sv, symbol_, side_, magic_enum::enum_name(state_));
}

// XXX REMEMBER to track and check average price
// XXX REMEMBER to round to min_trade_vol
void Instrument::refresh_positions(double completion) {
  auto reference_quantity = completion * total_quantity_;                        // 2
  auto aggress_quantity = std::max(reference_quantity - traded_quantity_, 0.0);  // 2
  auto target_quantity = std::max((total_quantity_ - traded_quantity_) - reference_quantity, 0.0);
  if (utils::compare(target_quantity, total_quantity_) > 0)
    log::fatal("HERE"sv);
  if (utils::update(target_quantity_, target_quantity))
    modify_quantity();

  // completed
  // working (passive)
  // working (aggressive)
  //
  // assert(completed <= reference)
  //
  // reference == completed ==> continue as-is
  //
  // working (passive) < completed:
  //   modify quantity DOWN <<== ALWAYS
  //   send aggressive order
  //
  // XXX TODO aggress
}

// DEBUG

void Instrument::DEBUG_print() {
  log::info(
      "[{}:{}] order_price={}, target_price={}, impact_price={}, top_of_book={}"sv, symbol_, side_, order_price_, target_price_, impact_price_, top_of_book_);
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
