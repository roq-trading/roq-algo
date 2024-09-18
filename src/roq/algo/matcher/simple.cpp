/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/matcher/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/update.hpp"

#include "roq/market/utils.hpp"

#include "roq/market/mbp/factory.hpp"

#include "roq/market/mbo/factory.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace matcher {

// === HELPERS ===

namespace {
auto create_market_by_price(auto &instrument) {
  return market::mbp::Factory::create(instrument.exchange, instrument.symbol);
}

auto create_market_by_order(auto &instrument) {
  return market::mbo::Factory::create(instrument.exchange, instrument.symbol);
}

// note! first is price which is used for the primary ordering, second is order_id which gives us priority

auto compare_buy = [](auto &lhs, auto &rhs) {
  if (lhs.first > rhs.first)
    return true;
  if (lhs.first == rhs.first)
    return lhs.second < rhs.second;
  return false;
};

auto compare_sell = [](auto &lhs, auto &rhs) {
  if (lhs.first < rhs.first)
    return true;
  if (lhs.first == rhs.first)
    return lhs.second < rhs.second;
  return false;
};

void add_order_helper(auto &container, auto compare, auto order_id, auto price) {
  std::pair value{price, order_id};
  auto iter = std::lower_bound(std::begin(container), std::end(container), value, compare);
  if (iter != std::end(container)) {
    if ((*iter).first == price && (*iter).second == order_id) [[unlikely]]  // duplicate?
      log::fatal("Unexpected: internal error"sv);
    container.insert(iter, value);
  } else {
    container.emplace_back(value);
  }
}

auto remove_order_helper(auto &container, auto compare, auto order_id, auto price) {
  std::pair value{price, order_id};
  auto iter = std::lower_bound(std::begin(container), std::end(container), value, compare);
  if (iter == std::end(container) || (*iter).first != price || (*iter).second != order_id)
    return false;
  container.erase(iter);
  return true;
}

template <typename Callback>
void try_match_helper(auto &container, auto compare, auto best, auto &cache, Callback callback) {
  auto iter = std::begin(container);
  for (; iter != std::end(container); ++iter) {
    auto &[price, order_id] = *iter;
    if (!compare(price, best))
      break;
    if (cache.get_order(order_id, [&](auto &order) { callback(order); })) {
    } else {
      log::fatal("Unexpected: internal error"sv);
    }
  }
  container.erase(std::begin(container), iter);
}
}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(Dispatcher &dispatcher, Config const &config, Cache &cache)
    : dispatcher_{dispatcher}, source_{config.source}, cache_{cache}, market_by_price_{create_market_by_price(config.instrument)},
      market_by_order_{create_market_by_order(config.instrument)} {
}

void Simple::operator()(Event<ReferenceData> const &event) {
  auto &[message_info, reference_data] = event;
  dispatcher_(event);  // note! dispatch market data
  utils::update_max(exchange_time_utc_, reference_data.exchange_time_utc);
  top_of_book_(event);
  (*market_by_price_)(event);
  (*market_by_order_)(event);
  if (utils::update(tick_size_, event.value.tick_size)) {
    auto precision = market::increment_to_precision(tick_size_);
    if (static_cast<std::underlying_type<decltype(precision)>::type>(precision) < static_cast<std::underlying_type<decltype(precision_)>::type>(precision_))
      log::fatal("Unexpected"sv);  // note! we can't support loss of precision
    // XXX FIXME internal prices must now be scaled relatively...
    precision_ = precision;
  }
}

void Simple::operator()(Event<MarketStatus> const &event) {
  auto &[message_info, market_status] = event;
  dispatcher_(event);  // note! dispatch market data
  utils::update_max(exchange_time_utc_, market_status.exchange_time_utc);
  if (!market_status_(event))
    return;
}

void Simple::operator()(Event<TopOfBook> const &event) {
  auto &[message_info, top_of_book] = event;
  utils::update_max(exchange_time_utc_, top_of_book.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (TODO potential to overlay own orders here)
  if (!top_of_book_(event))
    return;
  if (source_ != Source::TOP_OF_BOOK)
    return;
  if (std::isnan(tick_size_))  // note! reference data not yet received
    return;
  Event event_2{message_info, top_of_book_.layer};
  (*this)(event_2);
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  auto &[message_info, market_by_price_update] = event;
  utils::update_max(exchange_time_utc_, market_by_price_update.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (TODO potential to overlay own orders here)
  (*market_by_price_)(event);
  if (source_ != Source::MARKET_BY_PRICE)
    return;
  Layer layer;
  (*market_by_price_).extract({&layer, 1}, true);
  Event event_2{message_info, layer};
  (*this)(event_2);
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  auto &[message_info, market_by_order_update] = event;
  utils::update_max(exchange_time_utc_, market_by_order_update.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (TODO potential to overlay own orders here)
  (*market_by_order_)(event);
  if (source_ != Source::MARKET_BY_ORDER)
    return;
  Layer layer;
  (*market_by_order_).extract_2({&layer, 1});
  Event event_2{message_info, layer};
  (*this)(event_2);
}

void Simple::operator()(Event<TradeSummary> const &event) {
  auto &[message_info, trade_summary] = event;
  utils::update_max(exchange_time_utc_, trade_summary.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (TODO potential to overlay own fills here)
}

void Simple::operator()(Event<StatisticsUpdate> const &event) {
  auto &[message_info, statistics_update] = event;
  utils::update_max(exchange_time_utc_, statistics_update.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (TODO potential to overlay own fills here)
}

void Simple::operator()(Event<CreateOrder> const &event, cache::Order &order) {
  auto &[message_info, create_order] = event;
  auto validate = [&]() -> Error {
    // note! simulator will already have validated the request and here we just need to validate based on what is supported here
    if (create_order.order_type != OrderType::LIMIT)
      return Error::INVALID_ORDER_TYPE;
    if (create_order.time_in_force != TimeInForce::GTC)
      return Error::INVALID_TIME_IN_FORCE;
    if (create_order.execution_instructions != Mask<ExecutionInstruction>{})
      return Error::INVALID_EXECUTION_INSTRUCTION;
    if (!std::isnan(create_order.stop_price))
      return Error::INVALID_STOP_PRICE;
    return {};
  };
  if (auto error = validate(); error != Error{}) {
    dispatch_order_ack(event, order, error);
  } else {
    dispatch_order_ack(event, order, {}, RequestStatus::ACCEPTED);
    auto [price, overflow] = market::price_to_ticks(create_order.price, tick_size_, precision_);
    if (overflow) [[unlikely]]
      log::fatal("Unexpected: overflow when converting price to internal representation"sv);
    add_order(order.order_id, order.side, price);
    if (is_aggressive(create_order.side, price)) {
      auto matched_price = [&]() -> double {
        switch (create_order.side) {
          using enum Side;
          [[unlikely]] case UNDEFINED:
            break;
          case BUY:
            return best_.external.second;
          case SELL:
            return best_.external.first;
        }
        log::fatal("Unexpected"sv);
      }();
      auto external_trade_id = fmt::format("trd-{}"sv, cache_.get_next_trade_id());
      auto fill = Fill{
          .exchange_time_utc = exchange_time_utc_,
          .external_trade_id = external_trade_id,
          .quantity = create_order.quantity,
          .price = matched_price,
          .liquidity = Liquidity::TAKER,
          .quote_quantity = NaN,
          .commission_quantity = NaN,
          .commission_currency = {},
      };
      dispatch_trade_update(message_info, order, fill);
      order.order_status = OrderStatus::COMPLETED;
    } else {
      order.order_status = OrderStatus::WORKING;
    }
    dispatch_order_update(message_info, order, UpdateType::SNAPSHOT);
  }
}

void Simple::operator()(Event<ModifyOrder> const &event, cache::Order &order) {
  dispatch_order_ack(event, order, Error::NOT_SUPPORTED);
}

void Simple::operator()(Event<CancelOrder> const &event, cache::Order &order) {
  auto &[message_info, cancel_order] = event;
  if (utils::is_order_complete(order.order_status)) {
    dispatch_order_ack(event, order, Error::TOO_LATE_TO_MODIFY_OR_CANCEL);
  } else {
    auto [price, overflow] = market::price_to_ticks(order.price, tick_size_, precision_);
    if (overflow) [[unlikely]]
      log::fatal("Unexpected: overflow when converting price to internal representation"sv);
    if (remove_order(order.order_id, order.side, price)) {
      order.order_status = OrderStatus::CANCELED;
      dispatch_order_ack(event, order, {}, RequestStatus::ACCEPTED);
    } else {
      dispatch_order_ack(event, order, Error::TOO_LATE_TO_MODIFY_OR_CANCEL, RequestStatus::REJECTED);
    }
    dispatch_order_update(message_info, order, UpdateType::INCREMENTAL);
  }
}

void Simple::operator()(Event<CancelAllOrders> const &) {
  log::fatal("NOT IMPLEMENTED"sv);
}

// market

void Simple::operator()(Event<Layer> const &event) {
  auto &[message_info, layer] = event;
  assert(!std::isnan(tick_size_));
  assert(precision_ != Precision{});
  auto convert = [this](auto price, auto default_value) {
    if (!std::isnan(price)) {
      auto [units, overflow] = market::price_to_ticks(price, tick_size_, precision_);
      if (!overflow)
        return units;
    }
    return default_value;
  };
  auto matched_order = [&](auto &order) {
    auto external_trade_id = fmt::format("trd-{}"sv, cache_.get_next_trade_id());
    auto fill = Fill{
        .exchange_time_utc = exchange_time_utc_,
        .external_trade_id = external_trade_id,
        .quantity = order.remaining_quantity,
        .price = order.price,
        .liquidity = Liquidity::MAKER,
        .quote_quantity = NaN,
        .commission_quantity = NaN,
        .commission_currency = {},
    };
    dispatch_trade_update(message_info, order, fill);
    order.order_status = OrderStatus::COMPLETED;
    dispatch_order_update(message_info, order, UpdateType::INCREMENTAL);
  };
  auto bid = convert(layer.bid_price, std::numeric_limits<int64_t>::min());
  if (utils::update(best_.units.first, bid)) {
    best_.external.first = layer.bid_price;
    try_match(Side::BUY, matched_order);
  }
  auto ask = convert(layer.ask_price, std::numeric_limits<int64_t>::max());
  if (utils::update(best_.units.second, ask)) {
    best_.external.second = layer.ask_price;
    try_match(Side::SELL, matched_order);
  }
}

// orders

template <typename T>
void Simple::dispatch_order_ack(Event<T> const &event, cache::Order const &order, Error error, RequestStatus request_status) {
  auto &[message_info, value] = event;
  auto get_request_status = [&]() {
    if (request_status != RequestStatus{})
      return request_status;
    if (error != Error{})
      return RequestStatus::REJECTED;
    return RequestStatus::ACCEPTED;
  };
  auto get_text = [&]() -> std::string_view {
    if (error != Error{})
      return magic_enum::enum_name(error);
    return {};
  };
  auto order_ack = OrderAck{
      .account = order.account,
      .order_id = order.order_id,
      .exchange = order.exchange,
      .symbol = order.symbol,
      .side = order.side,
      .position_effect = order.position_effect,
      .margin_mode = order.margin_mode,
      .request_type = utils::get_request_type<decltype(value)>(),
      .origin = Origin::EXCHANGE,
      .request_status = get_request_status(),
      .error = error,
      .text = get_text(),
      .request_id = {},
      .external_account = {},
      .external_order_id = {},
      .client_order_id = {},
      .quantity = order.quantity,
      .price = order.price,
      .stop_price = order.stop_price,
      .routing_id = {},
      .version = utils::get_version(value),
      .risk_exposure = NaN,
      .risk_exposure_change = NaN,
      .traded_quantity = order.traded_quantity,
      .round_trip_latency = {},
      .user = {},
  };
  create_event_and_dispatch(dispatcher_, message_info, order_ack);
}

void Simple::dispatch_order_update(MessageInfo const &message_info, cache::Order const &order, UpdateType update_type) {
  auto order_update = static_cast<OrderUpdate>(order);
  order_update.update_type = update_type;
  create_event_and_dispatch(dispatcher_, message_info, order_update);
}

void Simple::dispatch_trade_update(MessageInfo const &message_info, cache::Order const &order, Fill const &fill) {
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
      .client_order_id = {},
      .fills = {&fill, 1},
      .routing_id = {},
      .update_type = UpdateType::SNAPSHOT,
      .user = {},
  };
  create_event_and_dispatch(dispatcher_, message_info, trade_update);
}

// utils

bool Simple::is_aggressive(Side side, int64_t price) const {
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      break;
    case BUY:
      return price >= best_.units.second;
    case SELL:
      return price <= best_.units.first;
  }
  log::fatal("Unexpected"sv);
}

void Simple::add_order(uint64_t order_id, Side side, int64_t price) {
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      log::fatal("Unexpected"sv);
    case BUY:
      add_order_helper(buy_, compare_buy, order_id, price);
      break;
    case SELL:
      add_order_helper(sell_, compare_sell, order_id, price);
      break;
  }
}

bool Simple::remove_order(uint64_t order_id, Side side, int64_t price) {
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      break;
    case BUY:
      return remove_order_helper(buy_, compare_buy, order_id, price);
    case SELL:
      return remove_order_helper(sell_, compare_sell, order_id, price);
  }
  log::fatal("Unexpected"sv);
}

template <typename Callback>
void Simple::try_match(Side side, Callback callback) {
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      log::fatal("Unexpected"sv);
    case BUY: {
      auto compare = [](auto lhs, auto rhs) { return lhs < rhs; };
      try_match_helper(sell_, compare, best_.units.first, cache_, callback);
      break;
    }
    case SELL: {
      auto compare = [](auto lhs, auto rhs) { return lhs > rhs; };
      try_match_helper(buy_, compare, best_.units.second, cache_, callback);
      break;
    }
  }
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
