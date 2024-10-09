/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {
auto to_support_type(MarketDataSource market_data_source) -> SupportType {
  switch (market_data_source) {
    using enum MarketDataSource;
    case TOP_OF_BOOK:
      return SupportType::TOP_OF_BOOK;
    case MARKET_BY_PRICE:
      return SupportType::MARKET_BY_PRICE;
    case MARKET_BY_ORDER:
      return SupportType::MARKET_BY_ORDER;
  }
  log::fatal("Unexpected"sv);
}

template <typename R>
auto create_instruments(auto &config, auto market_data_source) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = std::size(config.instruments);
  assert(size >= 2);
  if (size < 2)
    log::fatal("Unexpected: len(config.instruments)={}"sv, size);
  for (auto &item : config.instruments)
    result.emplace_back(item, market_data_source);
  return result;
}

template <typename R>
auto create_sources(auto &instruments) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto max_source = [&]() {
    size_t result = {};
    for (auto &item : instruments)
      result = std::max<size_t>(result, item.source);
    return result;
  }();
  auto size = max_source + 1;
  // accounts
  using accounts_type = std::remove_cvref<decltype(result_type::value_type::accounts)>::type;
  std::vector<accounts_type> tmp_1(size);
  for (size_t i = 0; i < std::size(instruments); ++i) {
    auto &item = instruments[i];
    tmp_1[item.source].try_emplace(item.account);
  }
  // instruments
  using instruments_type = std::remove_cvref<decltype(result_type::value_type::instruments)>::type;
  std::vector<instruments_type> tmp_2(size);
  for (size_t i = 0; i < std::size(instruments); ++i) {
    auto &item = instruments[i];
    tmp_2[item.source][item.exchange][item.symbol] = i;
  }
  // source
  for (size_t i = 0; i < size; ++i) {
    auto &accounts = tmp_1[i];
    auto &instruments = tmp_2[i];
    result.emplace_back(std::move(accounts), std::move(instruments));
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(strategy::Dispatcher &dispatcher, Config const &config, Cache &cache)
    : dispatcher_{dispatcher}, strategy_id_{config.strategy_id}, max_age_{config.max_age}, market_data_type_{to_support_type(config.market_data_source)},
      threshold_{config.threshold}, quantity_0_{config.quantity_0}, min_position_0_{config.min_position_0}, max_position_0_{config.max_position_0},
      cache_{cache}, instruments_{create_instruments<decltype(instruments_)>(config, config.market_data_source)},
      sources_{create_sources<decltype(sources_)>(instruments_)} {
  assert(!std::empty(instruments_));
  assert(!std::empty(sources_));
}

void Simple::operator()(Event<Timer> const &event) {
  check(event);
  auto &[message_info, timer] = event;
  assert(timer.now > 0ns);
  // XXX TODO process delayed order requests
}

void Simple::operator()(Event<Connected> const &event) {
  check(event);
  auto &[message_info, connected] = event;
  log::warn("[{}:{}] connected"sv, message_info.source, message_info.source_name);
}

void Simple::operator()(Event<Disconnected> const &event) {
  check(event);
  auto &[message_info, disconnected] = event;
  log::warn("[{}:{}] disconnected"sv, message_info.source, message_info.source_name);
  // reset source
  auto &source = sources_[message_info.source];
  source.ready = false;
  for (auto &[name, account] : source.accounts)
    account.has_download_orders = {};
  // note! no need to reset stream latencies
  // XXX FIXME TODO what about working orders ???
  // reset instruments
  auto callback = [](auto &instrument) {
    instrument.order_state = {};
    instrument.order_id = {};
  };
  get_instruments_by_source(event, callback);
}

void Simple::operator()(Event<DownloadEnd> const &event) {
  check(event);
  auto &[message_info, download_end] = event;
  auto &source = sources_[message_info.source];
  if (source.accounts.find(download_end.account) != std::end(source.accounts))
    max_order_id_ = std::max(download_end.max_order_id, max_order_id_);
}

void Simple::operator()(Event<Ready> const &event) {
  check(event);
  auto &[message_info, ready] = event;
  auto &source = sources_[message_info.source];
  source.ready = true;
  for (auto &[name, account] : source.accounts)
    if (account.has_download_orders) {
      // XXX FIXME TODO cancel by instrument
      auto cancel_all_orders = CancelAllOrders{
          .account = name,
          .order_id = {},
          .exchange = {},
          .symbol = {},
          .strategy_id = strategy_id_,
          .side = {},
      };
      try {
        log::debug("cancel_all_orders={}, source={}"sv, cancel_all_orders, message_info.source);
        dispatcher_.send(cancel_all_orders, message_info.source);
      } catch (NotReady &) {
      }
      account.has_download_orders = false;  // note! don't wait for ack
    }
}

void Simple::operator()(Event<StreamStatus> const &event) {
  check(event);
  auto &[message_info, stream_status] = event;
  // XXX FIXME TODO we need better (easier) identification of the primary feed
  if (stream_status.supports.has(market_data_type_) && stream_status.priority == Priority::PRIMARY) {
    auto callback = []([[maybe_unused]] auto &instrument) {};  // XXX TODO option to do something if stream status changes
    get_instruments_by_source(event, callback);
  }
}

// XXX FIXME TODO this is the real update (**NOT** correlated with simulated latency assumptions)
void Simple::operator()(Event<ExternalLatency> const &event) {
  check(event);
  auto &[message_info, external_latency] = event;
  auto &source = sources_[message_info.source];
  assert(external_latency.stream_id > 0);
  auto index = external_latency.stream_id - 1;
  source.stream_latency.resize(std::max<size_t>(index + 1, std::size(source.stream_latency)));
  source.stream_latency[index] = external_latency.latency;  // XXX TODO smooth
}

void Simple::operator()(Event<GatewayStatus> const &event) {
  check(event);
  auto &[message_info, gateway_status] = event;
  auto &source = sources_[message_info.source];
  if (source.accounts.find(gateway_status.account) != std::end(source.accounts)) {
    // XXX TODO use available?
  }
  update(message_info);
}

void Simple::operator()(Event<ReferenceData> const &event) {
  check(event);
  auto callback = [&](auto &instrument) { instrument(event); };
  get_instrument(event, callback);
}

void Simple::operator()(Event<MarketStatus> const &event) {
  check(event);
  auto &[message_info, market_status] = event;
  auto callback = [&](auto &instrument) {
    instrument(event);
    update(message_info);
  };
  get_instrument(event, callback);
}

void Simple::operator()(Event<TopOfBook> const &event) {
  check(event);
  auto &[message_info, top_of_book] = event;
  auto callback = [&](auto &instrument) {
    instrument(event);
    update(message_info);
  };
  get_instrument(event, callback);
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  check(event);
  auto &[message_info, market_by_price_update] = event;
  auto callback = [&](auto &instrument) {
    instrument(event);
    update(message_info);
  };
  get_instrument(event, callback);
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  check(event);
  auto &[message_info, market_by_order_update] = event;
  auto callback = [&](auto &instrument) {
    instrument(event);
    update(message_info);
  };
  get_instrument(event, callback);
}

// XXX FIXME TODO manage reject
void Simple::operator()(Event<OrderAck> const &event, cache::Order const &) {
  check(event);
  auto &[message_info, order_ack] = event;
  auto &source = sources_[message_info.source];
  assert(source.ready);
  auto callback = [&]([[maybe_unused]] auto &account, [[maybe_unused]] auto &instrument) {
    switch (order_ack.origin) {
      using enum Origin;
      case UNDEFINED:
        assert(false);
        break;
      case CLIENT:
      case GATEWAY:
        assert(order_ack.round_trip_latency == 0ns);
        break;
      case BROKER:
      case EXCHANGE:
        assert(order_ack.round_trip_latency > 0ns);
        break;
    }
  };
  if (!get_account_and_instrument(event, callback))
    log::fatal("Unexepcted: order_ack={}"sv, order_ack);
}

void Simple::operator()(Event<OrderUpdate> const &event, cache::Order const &) {
  check(event);
  auto &[message_info, order_update] = event;
  if (strategy_id_ && strategy_id_ != order_update.strategy_id)
    return;
  auto &source = sources_[message_info.source];
  auto is_order_complete = utils::is_order_complete(order_update.order_status);
  auto callback = [&](auto &account, auto &instrument) {
    if (source.ready) {  // live
      if (is_order_complete) {
        assert(instrument.order_state != OrderState::IDLE);
        instrument.order_state = {};
        instrument.order_id = {};
      } else {
        if (instrument.order_state == OrderState::CREATE) {
          instrument.order_state = OrderState::WORKING;
          assert(instrument.order_id);
        } else {
          // XXX FIXME cancel ???
        }
      }
    } else {  // download
      if (!is_order_complete) {
        account.has_download_orders = true;  // XXX FIXME TODO by instrument?
      }
    }
  };
  get_account_and_instrument(event, callback);
}

void Simple::operator()(Event<TradeUpdate> const &event, cache::Order const &) {
  check(event);
  auto &[message_info, trade_update] = event;
  if (strategy_id_ && strategy_id_ != trade_update.strategy_id)
    return;
  auto callback = [&]([[maybe_unused]] auto &account, [[maybe_unused]] auto &instrument) {
    // XXX FIXME TODO own position tracking (sum of fills)
  };
  get_account_and_instrument(event, callback);
}

// XXX FIXME TODO simulator doesn't emit snapshot ???
void Simple::operator()(Event<PositionUpdate> const &event) {
  check(event);
  auto &[message_info, position_update] = event;
  auto callback = [&]([[maybe_unused]] auto &account, auto &instrument) {
    instrument.position = position_update.long_quantity - position_update.short_quantity;
  };
  get_account_and_instrument(event, callback);
}

// utils

template <typename Callback>
void Simple::get_instruments_by_source(MessageInfo const &message_info, Callback callback) {
  if (message_info.source >= std::size(sources_)) [[unlikely]]
    log::fatal(R"(Unexpected: source={})"sv, message_info.source);
  auto &source = sources_[message_info.source];
  for (auto &[exchange, tmp] : source.instruments)
    for (auto &[symbol, index] : tmp) {
      auto &instrument = instruments_[index];
      callback(instrument);
    }
}

template <typename T, typename Callback>
bool Simple::get_instrument(Event<T> const &event, Callback callback) {
  auto &[message_info, value] = event;
  if (message_info.source >= std::size(sources_)) [[unlikely]]
    log::fatal(R"(Unexpected: source={})"sv, message_info.source);
  auto &source = sources_[message_info.source];
  auto iter_1 = source.instruments.find(value.exchange);
  if (iter_1 == std::end(source.instruments))
    return false;
  auto &tmp = (*iter_1).second;
  auto iter_2 = tmp.find(value.symbol);
  if (iter_2 == std::end(tmp))
    return false;
  auto &instrument = instruments_[(*iter_2).second];
  callback(instrument);
  return true;
}

template <typename T, typename Callback>
bool Simple::get_account_and_instrument(Event<T> const &event, Callback callback) {
  auto &[message_info, value] = event;
  if (message_info.source >= std::size(sources_)) [[unlikely]]
    log::fatal(R"(Unexpected: source={})"sv, message_info.source);
  auto &source = sources_[message_info.source];
  auto iter_1 = source.accounts.find(value.account);
  if (iter_1 == std::end(source.accounts))
    return false;
  auto &account = (*iter_1).second;
  auto iter_2 = source.instruments.find(value.exchange);
  if (iter_2 == std::end(source.instruments))
    return false;
  auto &tmp = (*iter_2).second;
  auto iter_3 = tmp.find(value.symbol);
  if (iter_3 == std::end(tmp))
    return false;
  auto &instrument = instruments_[(*iter_3).second];
  callback(account, instrument);
  return true;
}

template <typename T>
void Simple::update_latency(std::chrono::nanoseconds &latency, Event<T> const &event) {
  auto &[message_info, value] = event;
  auto base = [&]() {
    if (value.exchange_time_utc.count())
      return value.exchange_time_utc;
    if (value.sending_time_utc.count())
      return value.sending_time_utc;
    log::fatal("Unexpected: requires exchange_time_utc or sending_time_utc"sv);
  }();
  latency = message_info.receive_time_utc - base;
}

void Simple::update(MessageInfo const &message_info) {
  for (size_t i = 0; i < std::size(instruments_); ++i) {
    log::debug("instrument[{}]={}"sv, i, instruments_[i]);
    // XXX FIXME TODO check order state => maybe cancel or timeout
  }
  for (size_t i = 0; i < (std::size(instruments_) - 1); ++i) {
    auto &lhs = instruments_[i];
    auto lhs_ready = is_ready(message_info, lhs);
    auto &lhs_best = lhs.get_best();
    for (size_t j = i + 1; j < std::size(instruments_); ++j) {
      auto &rhs = instruments_[j];
      auto rhs_ready = is_ready(message_info, rhs);
      auto &rhs_best = rhs.get_best();
      auto spread_1 = lhs_best.bid_price - rhs_best.ask_price;  // sell lhs, buy rhs
      auto spread_2 = rhs_best.bid_price - lhs_best.ask_price;  // sell rhs, buy lhs
      auto trigger_1 = threshold_ < spread_1;
      auto trigger_2 = threshold_ < spread_2;
      // log::debug("[{}][{}] {} ({}) {} ({})"sv, i, j, spread_1, trigger_1, spread_2, trigger_2);
      if (lhs_ready && rhs_ready) {
        // XXX FIXME TODO something to decide whether to buy or sell (based on positions? or liquidity?)
        if (trigger_1)
          maybe_trade(message_info, Side::SELL, lhs, rhs);
        if (trigger_2)
          maybe_trade(message_info, Side::BUY, lhs, rhs);
      }
    }
  }
}

bool Simple::is_ready(MessageInfo const &message_info, Instrument const &instrument) const {
  if (instrument.order_state != OrderState::IDLE)
    return false;
  assert(instrument.order_id == 0);
  // XXX FIXME TODO check rate-limit throttling
  auto has_liquidity = [](auto price, auto quantity) { return !std::isnan(price) && !std::isnan(quantity) && quantity > 0.0; };
  auto &best = instrument.get_best();
  return instrument.is_market_active(message_info, max_age_) && has_liquidity(best.bid_price, best.bid_quantity) &&
         has_liquidity(best.ask_price, best.ask_quantity);
}

bool Simple::can_trade(Side side, Instrument &instrument) {
  switch (side) {
    using enum Side;
    case UNDEFINED:
      assert(false);
      break;
    case BUY:
      return utils::compare(instrument.position, max_position_0_) < 0;  // XXX FIXME TODO scale to instrument
    case SELL:
      return utils::compare(instrument.position, min_position_0_) > 0;  // XXX FIXME TODO scale to instrument
  }
  return false;
}

// XXX FIXME TODO support TimeInForce::IOC
void Simple::maybe_trade(MessageInfo const &, Side side, Instrument &lhs, Instrument &rhs) {
  auto helper = [this](auto side, auto &instrument) -> bool {
    assert(instrument.order_state == OrderState::IDLE);
    assert(instrument.order_id == 0);
    instrument.order_state = OrderState::CREATE;
    instrument.order_id = ++max_order_id_;
    auto price = utils::price_from_side(instrument.get_best(), utils::invert(side));  // aggress other side
    auto create_order = CreateOrder{
        .account = instrument.account,
        .order_id = instrument.order_id,
        .exchange = instrument.exchange,
        .symbol = instrument.symbol,
        .side = side,
        .position_effect = {},  // XXX TODO instrument config
        .margin_mode = {},      // XXX TODO instrument config
        .max_show_quantity = NaN,
        .order_type = OrderType::LIMIT,
        .time_in_force = TimeInForce::GTC,
        .execution_instructions = {},
        .request_template = {},
        .quantity = 1.0,  // XXX TODO
        .price = price,
        .stop_price = NaN,
        .routing_id = {},
        .strategy_id = strategy_id_,
    };
    log::debug("[{}] create_order={}"sv, instrument.source, create_order);
    try {
      dispatcher_.send(create_order, instrument.source);
      // XXX FIXME TODO record timestamp so we can rate-limit
    } catch (NotReady &) {
      instrument.order_state = OrderState::IDLE;
      instrument.order_id = {};
      return false;
    }
    return true;
  };
  if (can_trade(side, lhs) && helper(side, lhs)) {
    // note! only try second leg when first succeeds
    if (!helper(utils::invert(side), rhs)) {
      // note! we should try-cancel first leg when second fails
      // XXX FIXME TODO cancel first leg
    }
  }
}

template <typename T>
void Simple::check(Event<T> const &event) {
  using value_type = std::remove_cvref<T>::type;
  auto &[message_info, value] = event;
  auto helper = [](auto &lhs, auto rhs) {
    std::chrono::nanoseconds result;
    if (lhs.count()) {
      result = rhs - lhs;
    } else {
      result = {};
    }
    lhs = rhs;
    return result;
  };
  auto diff = helper(last_receive_time_, message_info.receive_time);
  auto diff_utc = helper(last_receive_time_utc_, message_info.receive_time_utc);  // XXX FIXME TODO track by source
  log::debug(
      "[{}:{}] receive_time={}({}), receive_time_utc={}({}), {}={}"sv,
      message_info.source,
      message_info.source_name,
      message_info.receive_time,
      diff,
      message_info.receive_time_utc,
      diff_utc,
      get_name<T>(),
      value);
  assert(!std::empty(message_info.source_name) || message_info.source == SOURCE_SELF);
  assert(message_info.receive_time.count());
  assert(diff >= 0ns);
  if constexpr (std::is_same<value_type, Connected>::value) {
    // XXX FIXME simulator doesn't populate receive_time_utc
  } else {
    assert(message_info.receive_time_utc.count());
    // note! diff_utc can be negative (clock adjustment, sampling from different cores, etc.)
  }
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
