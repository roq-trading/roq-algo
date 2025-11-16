/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include "roq/algo/matcher/queue_position_simple.hpp"

#include <magic_enum/magic_enum.hpp>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/update.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace matcher {

// trades => reduce
// quote => min
// do we need to count our quantity?

// === HELPERS ===

namespace {
// note! priority is preserved by first ordering by price (internal) and then by order_id
auto compare_buy_orders = [](auto &lhs, auto &rhs) {
  if (lhs.price > rhs.price) {
    return true;
  }
  if (lhs.price == rhs.price) {
    return lhs.order_id < rhs.order_id;
  }
  return false;
};

// note! priority is preserved by first ordering by price (internal) and then by order_id
auto compare_sell_orders = [](auto &lhs, auto &rhs) {
  if (lhs.price < rhs.price) {
    return true;
  }
  if (lhs.price == rhs.price) {
    return lhs.order_id < rhs.order_id;
  }
  return false;
};

void add_order_helper(auto &container, auto &&order, auto compare) {
  auto iter = std::lower_bound(std::begin(container), std::end(container), order, compare);
  if (iter != std::end(container)) {
    if ((*iter).price == order.price && (*iter).order_id == order.order_id) [[unlikely]] {
      log::fatal("Unexpected: internal error"sv);  // duplicate
    }
    container.insert(iter, order);
  } else {
    container.emplace_back(order);
  }
}

auto remove_order_helper(auto &container, auto &order, auto compare) {
  auto iter = std::lower_bound(std::begin(container), std::end(container), order, compare);
  if (iter == std::end(container) || (*iter).price != order.price || (*iter).order_id != order.order_id) {
    return false;
  }
  container.erase(iter);
  return true;
}

template <typename Callback>
void try_match_helper(auto &container, auto compare, auto top_of_book, auto &cache, Callback callback) {
  auto iter = std::begin(container);
  for (; iter != std::end(container); ++iter) {
    auto &order = *iter;
    if (!compare(order.price, top_of_book)) {
      break;
    }
    if (cache.get_order(order.order_id, [&](auto &order) { callback(order); })) {
    } else {
      log::fatal("Unexpected: internal error"sv);
    }
  }
  container.erase(std::begin(container), iter);
}
}  // namespace

// === IMPLEMENTATION ===

QueuePositionSimple::QueuePositionSimple(Dispatcher &dispatcher, OrderCache &order_cache, Config const &config)
    : dispatcher_{dispatcher}, order_cache_{order_cache}, market_data_{config.exchange, config.symbol, config.market_data_source} {
  if (config.market_data_source == MarketDataSource::TOP_OF_BOOK) {
    throw RuntimeError{"Unsupported: market_data_source={}"sv, config.market_data_source};
  }
}

// note! the following handlers **must** dispatch market data and **may** potentially overlay own orders and fills

void QueuePositionSimple::operator()(Event<ReferenceData> const &event) {
  check(event);
  dispatcher_(event);  // note!
  market_data_(event);
}

void QueuePositionSimple::operator()(Event<MarketStatus> const &event) {
  check(event);
  dispatcher_(event);  // note!
  market_data_(event);
}

void QueuePositionSimple::operator()(Event<TopOfBook> const &event) {
  check(event);
  dispatcher_(event);  // note!
  if (market_data_(event)) {
    match_resting_orders(event);
  }
}

void QueuePositionSimple::operator()(Event<MarketByPriceUpdate> const &event) {
  check(event);
  dispatcher_(event);  // note!
  if (market_data_(event)) {
    // HANS here we should just iterate our own orders (cost will be smaller) and update min()
    // --- maybe some hash with working order prices? and mark dirty?
    match_resting_orders(event);
  }
}

void QueuePositionSimple::operator()(Event<MarketByOrderUpdate> const &event) {
  check(event);
  dispatcher_(event);  // note!
  if (market_data_(event)) {
    match_resting_orders(event);
  }
}

void QueuePositionSimple::operator()(Event<TradeSummary> const &event) {
  check(event);
  dispatcher_(event);  // note!
  if (market_data_(event)) {
    match_resting_orders_2(event);
  }
}

void QueuePositionSimple::operator()(Event<StatisticsUpdate> const &event) {
  check(event);
  dispatcher_(event);  // note!
  market_data_(event);
}

void QueuePositionSimple::operator()(Event<CreateOrder> const &event, cache::Order &order) {
  check(event);
  auto &[message_info, create_order] = event;
  auto validate = [&]() -> Error {
    // note! simulator will already have validated the request and here we just need to validate based on what is supported here
    if (create_order.quantity_type != QuantityType{}) {
      return Error::INVALID_QUANTITY_TYPE;
    }
    if (create_order.order_type != OrderType::LIMIT) {
      return Error::INVALID_ORDER_TYPE;
    }
    if (create_order.time_in_force != TimeInForce::GTC) {
      return Error::INVALID_TIME_IN_FORCE;
    }
    if (create_order.execution_instructions != Mask<ExecutionInstruction>{}) {
      return Error::INVALID_EXECUTION_INSTRUCTION;
    }
    if (!std::isnan(create_order.stop_price)) {
      return Error::INVALID_STOP_PRICE;
    }
    return {};
  };
  if (auto error = validate(); error != Error{}) {
    dispatch_order_ack(event, order, error);
  } else {
    dispatch_order_ack(event, order, {}, RequestStatus::ACCEPTED);
    auto [price, overflow] = market_data_.price_to_ticks(create_order.price);
    if (overflow) [[unlikely]] {
      log::fatal("Unexpected: overflow when converting price to internal representation"sv);
    }
    if (is_aggressive(create_order.side, price)) {
      auto matched_price = [&]() -> double {
        switch (create_order.side) {
          using enum Side;
          [[unlikely]] case UNDEFINED:
            break;
          case BUY:
            return top_of_book_.external.ask_price;
          case SELL:
            return top_of_book_.external.bid_price;
        }
        log::fatal("Unexpected"sv);
      }();
      auto external_trade_id = fmt::format("trd-{}"sv, order_cache_.get_next_trade_id());
      auto fill = Fill{
          .exchange_time_utc = market_data_.exchange_time_utc(),
          .external_trade_id = external_trade_id,
          .quantity = create_order.quantity,
          .price = matched_price,
          .liquidity = Liquidity::TAKER,
          .commission_amount = NaN,
          .commission_currency = {},
          .base_amount = NaN,
          .quote_amount = NaN,
          .profit_loss_amount = NaN,
      };
      order.create_time_utc = market_data_.exchange_time_utc();
      order.update_time_utc = market_data_.exchange_time_utc();
      order.order_status = OrderStatus::COMPLETED;
      order.remaining_quantity = 0.0;
      order.traded_quantity = fill.quantity;
      order.average_traded_price = fill.price;
      order.last_traded_quantity = fill.quantity;
      order.last_traded_price = fill.price;
      order.last_liquidity = fill.liquidity;
      dispatch_order_update(message_info, order);
      dispatch_trade_update(message_info, order, fill);
    } else {
      add_order(order.order_id, order.side, price);
      order.create_time_utc = market_data_.exchange_time_utc();
      order.update_time_utc = market_data_.exchange_time_utc();
      order.order_status = OrderStatus::WORKING;
      order.remaining_quantity = create_order.quantity;
      order.traded_quantity = 0.0;
      order.average_traded_price = NaN;
      order.last_traded_quantity = NaN;
      order.last_traded_price = NaN;
      order.last_liquidity = {};
      dispatch_order_update(message_info, order);
    }
  }
}

void QueuePositionSimple::operator()(Event<ModifyOrder> const &event, cache::Order &order) {
  check(event);
  dispatch_order_ack(event, order, Error::NOT_SUPPORTED);
}

void QueuePositionSimple::operator()(Event<CancelOrder> const &event, cache::Order &order) {
  check(event);
  auto &[message_info, cancel_order] = event;
  if (utils::is_order_complete(order.order_status)) {
    dispatch_order_ack(event, order, Error::TOO_LATE_TO_MODIFY_OR_CANCEL);
  } else {
    auto [price, overflow] = market_data_.price_to_ticks(order.price);
    if (overflow) [[unlikely]] {
      log::fatal("Unexpected: overflow when converting price to internal representation"sv);
    }
    if (remove_order(order.order_id, order.side, price)) {
      order.update_time_utc = market_data_.exchange_time_utc();
      order.order_status = OrderStatus::CANCELED;
      dispatch_order_ack(event, order, {}, RequestStatus::ACCEPTED);
    } else {
      dispatch_order_ack(event, order, Error::TOO_LATE_TO_MODIFY_OR_CANCEL, RequestStatus::REJECTED);
    }
    dispatch_order_update(message_info, order);
  }
}

void QueuePositionSimple::operator()(Event<CancelAllOrders> const &event) {
  check(event);
  log::fatal("NOT IMPLEMENTED"sv);
}

void QueuePositionSimple::operator()(Event<MassQuote> const &event) {
  check(event);
  log::fatal("NOT IMPLEMENTED"sv);
}

void QueuePositionSimple::operator()(Event<CancelQuotes> const &event) {
  check(event);
  log::fatal("NOT IMPLEMENTED"sv);
}

// market

void QueuePositionSimple::match_resting_orders(MessageInfo const &message_info) {
  if (!market_data_.has_tick_size()) {
    return;
  }
  auto convert = [this](auto price, auto default_value) {
    if (!std::isnan(price)) {
      auto [units, overflow] = market_data_.price_to_ticks(price);
      if (!overflow) {
        return units;
      }
    }
    return default_value;
  };
  auto matched_order = [&](auto &order) {
    assert(utils::compare(order.remaining_quantity, 0.0) > 0);
    auto external_trade_id = fmt::format("trd-{}"sv, order_cache_.get_next_trade_id());
    auto fill = Fill{
        .exchange_time_utc = market_data_.exchange_time_utc(),
        .external_trade_id = external_trade_id,
        .quantity = order.remaining_quantity,
        .price = order.price,
        .liquidity = Liquidity::MAKER,
        .commission_amount = NaN,
        .commission_currency = {},
        .base_amount = NaN,
        .quote_amount = NaN,
        .profit_loss_amount = NaN,
    };
    order.update_time_utc = market_data_.exchange_time_utc();
    order.order_status = OrderStatus::COMPLETED;
    order.remaining_quantity = 0.0;
    order.traded_quantity = fill.quantity;
    order.average_traded_price = fill.price;
    order.last_traded_quantity = fill.quantity;
    order.last_traded_price = fill.price;
    order.last_liquidity = fill.liquidity;
    dispatch_order_update(message_info, order);
    dispatch_trade_update(message_info, order, fill);
  };
  auto &top_of_book = market_data_.top_of_book();
  // HANS check min()
  auto bid = convert(top_of_book.bid_price, std::numeric_limits<int64_t>::min());
  if (utils::update(top_of_book_.internal.bid_price, bid)) {
    top_of_book_.external.bid_price = top_of_book.bid_price;
    try_match(Side::BUY, matched_order);
  }
  auto ask = convert(top_of_book.ask_price, std::numeric_limits<int64_t>::max());
  if (utils::update(top_of_book_.internal.ask_price, ask)) {
    top_of_book_.external.ask_price = top_of_book.ask_price;
    try_match(Side::SELL, matched_order);
  }
}

void QueuePositionSimple::match_resting_orders_2(Event<TradeSummary> const &event) {
  auto &[message_info, trade_summary] = event;
  assert(!std::empty(trade_summary.trades));
  auto helper = [&]([[maybe_unused]] auto price, [[maybe_unused]] auto total_quantity) {
    // HANS XXX TODO
    // like try_match with matched_order callback (reuse)
    // remember -- fills happen (mostly) near top of book
  };
  auto price = NaN;
  auto total_quantity = 0.0;
  for (auto &item : trade_summary.trades) {
    if (std::isnan(price) || utils::compare(price, item.price) != 0) {
      if (utils::compare(total_quantity, 0.0) > 0) {
        assert(!std::isnan(price));
        helper(price, total_quantity);
      }
      price = item.price;
      total_quantity = 0.0;
    }
  }
  assert(!std::isnan(price));
  assert(utils::compare(total_quantity, 0.0) > 0);
  helper(price, total_quantity);
}

// orders

template <typename T>
void QueuePositionSimple::dispatch_order_ack(Event<T> const &event, cache::Order const &order, Error error, RequestStatus request_status) {
  auto &[message_info, value] = event;
  auto get_request_status = [&]() {
    if (request_status != RequestStatus{}) {
      return request_status;
    }
    if (error != Error{}) {
      return RequestStatus::REJECTED;
    }
    return RequestStatus::ACCEPTED;
  };
  auto get_text = [&]() -> std::string_view {
    if (error != Error{}) {
      return magic_enum::enum_name(error);
    }
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
      .quantity_type = order.quantity_type,
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
      .leverage = order.leverage,
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

void QueuePositionSimple::dispatch_order_update(MessageInfo const &message_info, cache::Order &order) {
  if (order.create_time_utc.count() == 0) {
    order.create_time_utc = market_data_.exchange_time_utc();  // XXX FIXME TODO this is strategy create time (not exchange time)
  }
  order.update_time_utc = market_data_.exchange_time_utc();  // XXX FIXME TODO this is strategy receive time (not exchange time)
  auto order_update = static_cast<OrderUpdate>(order);
  order_update.sending_time_utc = market_data_.exchange_time_utc();
  order_update.update_type = UpdateType::INCREMENTAL;
  create_event_and_dispatch(dispatcher_, message_info, order_update);
}

void QueuePositionSimple::dispatch_trade_update(MessageInfo const &message_info, cache::Order const &order, Fill const &fill) {
  auto trade_update = TradeUpdate{
      .account = order.account,
      .order_id = order.order_id,
      .exchange = order.exchange,
      .symbol = order.symbol,
      .side = order.side,
      .position_effect = order.position_effect,
      .margin_mode = order.margin_mode,
      .quantity_type = order.quantity_type,
      .create_time_utc = market_data_.exchange_time_utc(),  // XXX FIXME TODO this is strategy receive time (not exchange time)
      .update_time_utc = market_data_.exchange_time_utc(),  // XXX FIXME TODO this is strategy receive time (not exchange time)
      .external_account = {},
      .external_order_id = {},
      .client_order_id = {},
      .fills = {&fill, 1},
      .routing_id = {},
      .update_type = UpdateType::INCREMENTAL,
      .user = {},
  };
  create_event_and_dispatch(dispatcher_, message_info, trade_update);
}

// utils

bool QueuePositionSimple::is_aggressive(Side side, int64_t price) const {
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      assert(false);
      break;
    case BUY:
      return price >= top_of_book_.internal.ask_price;
    case SELL:
      return price <= top_of_book_.internal.bid_price;
  }
  log::fatal("Unexpected"sv);
}

void QueuePositionSimple::add_order(uint64_t order_id, Side side, int64_t price) {
  auto quantity = market_data_.total_quantity(side, price);  // XXX FIXME TODO convert internal price to external
  auto order = Order{
      .order_id = order_id,
      .price = price,
      .ahead = quantity,
  };
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      assert(false);
      log::fatal("Unexpected"sv);
    case BUY:
      add_order_helper(buy_orders_, std::move(order), compare_buy_orders);
      break;
    case SELL:
      add_order_helper(sell_orders_, std::move(order), compare_sell_orders);
      break;
  }
}

bool QueuePositionSimple::remove_order(uint64_t order_id, Side side, int64_t price) {
  auto order = Order{
      .order_id = order_id,
      .price = price,
      .ahead = {},
  };
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      assert(false);
      break;
    case BUY:
      return remove_order_helper(buy_orders_, order, compare_buy_orders);
    case SELL:
      return remove_order_helper(sell_orders_, order, compare_sell_orders);
  }
  log::fatal("Unexpected"sv);
}

template <typename Callback>
void QueuePositionSimple::try_match(Side side, Callback callback) {
  switch (side) {
    using enum Side;
    [[unlikely]] case UNDEFINED:
      assert(false);
      log::fatal("Unexpected"sv);
    case BUY: {
      auto compare = [](auto lhs, auto rhs) { return lhs >= rhs; };
      try_match_helper(sell_orders_, compare, top_of_book_.internal.bid_price, order_cache_, callback);
      break;
    }
    case SELL: {
      auto compare = [](auto lhs, auto rhs) { return lhs <= rhs; };
      try_match_helper(buy_orders_, compare, top_of_book_.internal.ask_price, order_cache_, callback);
      break;
    }
  }
}

template <typename T>
void QueuePositionSimple::check(Event<T> const &event) {
  auto &[message_info, value] = event;
  log::debug(
      "[{}:{}] receive_time={}, receive_time_utc={}, {}={}"sv,
      message_info.source,
      message_info.source_name,
      message_info.receive_time,
      message_info.receive_time_utc,
      get_name<T>(),
      value);
  time_checker_(event);
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
