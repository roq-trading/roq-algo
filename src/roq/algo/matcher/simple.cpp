/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/matcher/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/update.hpp"

#include "roq/market/utils.hpp"

#include "roq/market/mbp/factory.hpp"

#include "roq/market/mbo/factory.hpp"

#include "roq/algo/matcher/utils.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace matcher {

// === CONSTANTS ===

namespace {
auto const DEFAULT_GATEWAY_SETTINGS = GatewaySettings{
    .supports = {},
    .mbp_max_depth = {},
    .mbp_tick_size_multiplier = NaN,
    .mbp_min_trade_vol_multiplier = NaN,
    .mbp_allow_remove_non_existing = true,  // note! client assumes server does validation
    .mbp_allow_price_inversion = true,      // note! client assumes server does validation
    .mbp_checksum = false,
    .oms_download_has_state = false,
    .oms_download_has_routing_id = false,
    .oms_request_id_type = {},
    .oms_cancel_all_orders = {},
};
}  // namespace

// === IMPLEMENTATION ===

namespace {
auto create_market_by_price(auto &exchange, auto &symbol) {
  return market::mbp::Factory::create(exchange, symbol, DEFAULT_GATEWAY_SETTINGS);
}

auto create_market_by_order(auto &exchange, auto &symbol) {
  return market::mbo::Factory::create(exchange, symbol);
}
}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(Matcher::Dispatcher &dispatcher, std::string_view const &exchange, std::string_view const &symbol, MatchingSource market_data_source)
    : dispatcher_{dispatcher}, matching_source_{market_data_source}, market_by_price_{create_market_by_price(exchange, symbol)},
      market_by_order_{create_market_by_order(exchange, symbol)} {
}

void Simple::operator()(Event<GatewaySettings> const &event) {
  dispatcher_(event);
}

void Simple::operator()(Event<StreamStatus> const &event) {
  dispatcher_(event);
}

// note! delayed extraction of accounts (to avoid config requirement)
void Simple::operator()(Event<GatewayStatus> const &event) {
  if (!std::empty(event.value.account))
    accounts_.emplace(event.value.account);
  dispatcher_(event);
}

void Simple::operator()(Event<ReferenceData> const &event) {
  dispatcher_(event);
  top_of_book_(event);
  (*market_by_price_)(event);
  (*market_by_order_)(event);
  if (utils::update(tick_size_, event.value.tick_size)) {
    auto precision = market::increment_to_precision(tick_size_);
    // resize
    precision_ = precision;
  }
}

void Simple::operator()(Event<MarketStatus> const &event) {
  dispatcher_(event);
  if (!market_status_(event))
    return;
}

void Simple::operator()(Event<TopOfBook> const &event) {
  dispatcher_(event);
  if (!top_of_book_(event))
    return;
  if (matching_source_ != MatchingSource::TOP_OF_BOOK)
    return;
  (*this)(top_of_book_.layer);
  try_match();
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  dispatcher_(event);  // another option would be to overlay our own orders before forwarding
  (*market_by_price_)(event);
  if (matching_source_ != MatchingSource::MARKET_BY_PRICE)
    return;
  Layer layer;
  (*market_by_price_).extract({&layer, 1}, true);
  (*this)(layer);
  try_match();
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  dispatcher_(event);  // another option would be to overlay our own orders before forwarding
  (*market_by_order_)(event);
  if (matching_source_ != MatchingSource::MARKET_BY_ORDER)
    return;
  log::fatal("NOT IMPLEMENTED"sv);
  // (*market_by_order_).extract({&best_, 1}, true);
  try_match();
}

void Simple::operator()(Event<TradeSummary> const &event) {
  dispatcher_(event);
}

void Simple::operator()(Event<StatisticsUpdate> const &event) {
  dispatcher_(event);
}

void Simple::operator()(Event<CreateOrder> const &event) {
  auto found = [&]([[maybe_unused]] auto &order) { dispatch_order_ack(event, Error::INVALID_ORDER_ID); };
  auto not_found = [&]() {
    auto &[message_info, create_order] = event;
    // validate
    if (accounts_.find(create_order.account) == std::end(accounts_)) {
      dispatch_order_ack(event, Error::INVALID_ACCOUNT);
      return;
    }
    if (create_order.order_type != OrderType::LIMIT) {
      dispatch_order_ack(event, Error::INVALID_ORDER_TYPE);
      return;
    }
    dispatch_order_ack(event, {}, RequestStatus::FORWARDED);
    // create
    auto order_id = get_order_id(event.value);
    assert(order_id > 0);
    auto res = order_.emplace(order_id, event);
    assert(res.second);
    auto &order = (*res.first).second;
    dispatch_order_ack(event, {}, RequestStatus::ACCEPTED);
    order.order_status = OrderStatus::WORKING;
    order.max_response_version = 1;
    order.max_accepted_version = 1;
    // match
    // XXX FIXME TODO execution instructions?
    auto units = market::price_to_ticks(create_order.price, tick_size_, precision_);
    if (can_match(create_order.side, create_order.price)) {
      // XXX FIXME TODO
    }
    dispatch_order_update(message_info, order);
  };
  if (find_order(event, found)) {
  } else {
    not_found();
  }
}

void Simple::operator()(Event<ModifyOrder> const &event) {
  dispatch_order_ack(event, Error::NOT_SUPPORTED);
}

void Simple::operator()(Event<CancelOrder> const &event) {
  auto found = [&](auto &order) {
    if (utils::is_order_complete(order.order_status)) {
      dispatch_order_ack(event, Error::TOO_LATE_TO_MODIFY_OR_CANCEL);
      return;
    }
    auto &[message_info, cancel_order] = event;
    auto version = cancel_order.version ? cancel_order.version : 2;
    dispatch_order_ack(event, order, {}, RequestStatus::FORWARDED);
    dispatch_order_ack(event, order, {}, RequestStatus::ACCEPTED);
    order.order_status = OrderStatus::CANCELED;
    order.max_request_version = version;
    dispatch_order_update(message_info, order);
    // XXX FIXME remove
  };
  auto not_found = [&]() { dispatch_order_ack(event, Error::INVALID_ORDER_ID); };
  if (find_order(event, found)) {
  } else {
    not_found();
  }
}

void Simple::operator()(Event<CancelAllOrders> const &) {
  log::fatal("NOT IMPLEMENTED"sv);
}

void Simple::operator()(Event<PositionUpdate> const &event) {
  dispatcher_(event);
}

void Simple::operator()(Event<FundsUpdate> const &event) {
  dispatcher_(event);
}

// market

void Simple::operator()(Layer const &layer) {
  auto convert = [this](auto price, auto default_value) {
    if (!std::isnan(price)) {
      auto [units, success] = market::price_to_ticks(price, tick_size_, precision_);
      if (success)
        return units;
    }
    return default_value;
  };
  best_ = std::make_pair<int64_t, int64_t>(
      convert(layer.bid_price, std::numeric_limits<int64_t>::min()), convert(layer.ask_price, std::numeric_limits<int64_t>::max()));
}

// order

template <typename T, typename Callback>
bool Simple::find_order(Event<T> const &event, Callback callback) {
  auto order_id = get_order_id(event.value);
  assert(order_id > 0);
  auto iter = order_.find(order_id);
  if (iter == std::end(order_))
    return false;
  callback((*iter).second);
  return true;
}

template <typename T>
void Simple::dispatch_order_ack(Event<T> const &event, Error error, RequestStatus request_status) {
  auto &[message_info, value] = event;
  auto order_ack = create_order_ack(value, error, request_status);
  create_event_and_dispatch(dispatcher_, message_info, order_ack);
}

template <typename T>
void Simple::dispatch_order_ack(Event<T> const &event, Order const &order, Error error, RequestStatus request_status) {
  auto &[message_info, value] = event;
  auto order_ack = create_order_ack(value, order, error, request_status);
  create_event_and_dispatch(dispatcher_, message_info, order_ack);
}

void Simple::dispatch_order_update(MessageInfo const &message_info, Order const &order) {
  auto order_update = static_cast<OrderUpdate>(order);
  order_update.update_type = UpdateType::SNAPSHOT;  // XXX check
  create_event_and_dispatch(dispatcher_, message_info, order_update);
}

void Simple::dispatch_trade_update(MessageInfo const &message_info, Order const &order, Fill const &fill) {
  /*
  auto fill = Fill{
      .exchange_time_utc = {},  // XXX FIXME
      .external_trade_id = {},  // XXX FIXME
      .quantity = quantity,
      .price = price,
      .liquidity = liquidity,
      .quote_quantity = NaN,
      .commission_quantity = NaN,
      .commission_currency = {},
  };
  */
  auto trade_update = TradeUpdate{
      .account = order.account,
      .order_id = order.order_id,
      .exchange = order.exchange,
      .symbol = order.symbol,
      .side = order.side,
      .position_effect = order.position_effect,
      .margin_mode = order.margin_mode,
      .create_time_utc = message_info.receive_time_utc,
      .update_time_utc = message_info.receive_time_utc,
      .external_account = {},
      .external_order_id = {},
      .client_order_id = {},  // XXX FIXME
      .fills = {&fill, 1},
      .routing_id = {},
      .update_type = UpdateType::SNAPSHOT,  // XXX check
      .user = {},
  };
  create_event_and_dispatch(dispatcher_, message_info, trade_update);
}

// order

void Simple::try_match() {
  // XXX FIXME TODO
}

bool Simple::can_match(Side side, int64_t price) const {
  return false;
  // XXX FIXME TODO
}

void Simple::add(uint64_t order_id, Side side, int64_t price) {
  // XXX FIXME TODO
}

void Simple::remove(uint64_t order_id, Side side, int64_t price) {
  // XXX FIXME TODO
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
