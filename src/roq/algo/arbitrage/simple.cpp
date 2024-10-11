/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/compare.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {
auto create_market_data_type(auto &config) -> SupportType {
  switch (config.market_data_source) {
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
auto create_instruments(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = std::size(config.instruments);
  assert(size >= 2);
  if (size < 2)
    log::fatal("Unexpected: len(config.instruments)={}"sv, size);
  for (auto &item : config.instruments)
    result.emplace_back(item, config.market_data_source);
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

Simple::Simple(strategy::Dispatcher &dispatcher, Config const &config, OrderCache &order_cache)
    : dispatcher_{dispatcher}, strategy_id_{config.strategy_id}, max_age_{config.max_age}, market_data_type_{create_market_data_type(config)},
      threshold_{config.threshold}, quantity_0_{config.quantity_0}, min_position_0_{config.min_position_0}, max_position_0_{config.max_position_0},
      order_cache_{order_cache}, instruments_{create_instruments<decltype(instruments_)>(config)}, sources_{create_sources<decltype(sources_)>(instruments_)} {
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
  // XXX TODO maybe cancel working orders on other sources?
  // reset instruments
  auto callback = [&](auto &instrument) { instrument(event); };
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
  log::warn("[{}:{}] ready"sv, message_info.source, message_info.source_name);
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
      account.has_download_orders = false;  // note! don't bother waiting for the ack
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

// note! this is only a sample, typically based on ping-pong between gateway and some exchange end-point
// XXX FIXME TODO this is currently the raw event-log event (**NOT** correlated with simulated latency assumptions)
void Simple::operator()(Event<ExternalLatency> const &event) {
  check(event);
  auto &[message_info, external_latency] = event;
  auto &source = sources_[message_info.source];
  assert(external_latency.stream_id > 0);
  auto index = external_latency.stream_id - 1;
  source.stream_latency.resize(std::max<size_t>(index + 1, std::size(source.stream_latency)));
  source.stream_latency[index] = external_latency.latency;  // XXX TODO exponential smoothing
}

void Simple::operator()(Event<GatewayStatus> const &event) {
  check(event);
  auto &[message_info, gateway_status] = event;
  auto &source = sources_[message_info.source];
  if (source.accounts.find(gateway_status.account) != std::end(source.accounts)) {
    // XXX TODO use gateway_status.available to monitor the gateway's order management availability (perhaps not connected)
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
  auto callback = [&](auto &instrument) {
    if (instrument(event))
      update(event);
  };
  get_instrument(event, callback);
}

void Simple::operator()(Event<TopOfBook> const &event) {
  check(event);
  auto callback = [&](auto &instrument) {
    if (instrument(event))
      update(event);
  };
  get_instrument(event, callback);
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  check(event);
  auto callback = [&](auto &instrument) {
    if (instrument(event))
      update(event);
  };
  get_instrument(event, callback);
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  check(event);
  auto callback = [&](auto &instrument) {
    if (instrument(event))
      update(event);
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
        assert(order_ack.round_trip_latency > 0ns);  // note! this is the request round-trip latency between gateway and exchange
        break;
    }
  };
  if (!get_account_and_instrument(event, callback))
    log::fatal("Unexepcted: order_ack={}"sv, order_ack);
}

void Simple::operator()(Event<OrderUpdate> const &event, cache::Order const &) {
  check(event);
  if (is_my_order(event)) {
    auto &[message_info, order_update] = event;
    auto &source = sources_[message_info.source];
    auto is_order_complete = utils::is_order_complete(order_update.order_status);
    auto callback = [&](auto &account, auto &instrument) {
      switch (order_update.update_type) {
        using enum UpdateType;
        case UNDEFINED:
          assert(false);
          break;
        case SNAPSHOT:
          if (source.ready) {
            // note! gateway has reconnected to the exchange and we are now receiving the downloaded state of managed orders
            // XXX FIXME TODO not sure what...
          } else {
            // note! we have (re)connected to gateway and managed orders are now being downloaded
            if (!is_order_complete) {
              // note! we cancel all downloaded orders when we (re)connect to the gateway (see ready event)
              account.has_download_orders = true;  // XXX FIXME TODO manage this by instrument
            }
          }
          break;
        case INCREMENTAL: {
          // note! gateway has received an order update directly from the exchange
          assert(source.ready);
          if (is_order_complete) {
            assert(instrument.order_state != OrderState::IDLE);
            instrument.reset();
          } else {
            switch (instrument.order_state) {
              using enum OrderState;
              case IDLE:
                assert(false);
                break;
              case CREATE:
                instrument.order_state = OrderState::WORKING;
                assert(instrument.order_id);
                break;
              case WORKING:
                assert(instrument.order_id);
                break;
              case CANCEL:
                // note! **not** a hard fault because there is a race + the cancel request could be rejected (or even lost)
                // XXX FIXME TODO maybe retry ???
                assert(instrument.order_id);
            }
          }
          break;
        }
        case STALE:
          // note! gateway has lost connection to exchange and we get notified ==> wait for gateway to reconnect and send snapshot
          break;
      }
    };
    get_account_and_instrument(event, callback);
  }
}

void Simple::operator()(Event<TradeUpdate> const &event, cache::Order const &) {
  check(event);
  if (is_my_order(event)) {
    auto callback = [&]([[maybe_unused]] auto &account, auto &instrument) { instrument(event); };
    get_account_and_instrument(event, callback);
  }
}

void Simple::operator()(Event<PositionUpdate> const &event) {
  check(event);
  auto callback = [&]([[maybe_unused]] auto &account, auto &instrument) { instrument(event); };
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
    // XXX FIXME TODO check order state => maybe cancel or timeout?
  }
  for (size_t i = 0; i < (std::size(instruments_) - 1); ++i) {
    auto &lhs = instruments_[i];
    if (lhs.is_ready(message_info, max_age_)) {
      for (size_t j = i + 1; j < std::size(instruments_); ++j) {
        auto &rhs = instruments_[j];
        if (rhs.is_ready(message_info, max_age_))
          check_spread(message_info, lhs, rhs);
      }
    }
  }
  for (auto &item : instruments_)
    publish_statistics(item);
}

void Simple::check_spread(MessageInfo const &message_info, Instrument &lhs, Instrument &rhs) {
  auto [bid_0, ask_0] = lhs.get_best();
  auto [bid_1, ask_1] = rhs.get_best();
  auto spread_0 = bid_0 - ask_1;  // note! sell(lhs), buy(rhs)
  auto spread_1 = bid_1 - ask_0;  // note! buy(lhs), sell(rhs)
  auto trigger_0 = threshold_ < spread_0;
  auto trigger_1 = threshold_ < spread_1;
  log::debug(
      "SPREAD [{}:{}:{}][{}:{}:{}] {} ({}) {} ({})"sv,
      lhs.source,
      lhs.exchange,
      lhs.symbol,
      rhs.source,
      rhs.exchange,
      rhs.symbol,
      spread_0,
      trigger_0,
      spread_1,
      trigger_1);
  if (trigger_0)
    maybe_trade_spread(message_info, Side::SELL, lhs, rhs);
  if (trigger_1)
    maybe_trade_spread(message_info, Side::BUY, lhs, rhs);
}

void Simple::maybe_trade_spread(MessageInfo const &, Side side, Instrument &lhs, Instrument &rhs) {
  auto helper = [this](auto side, auto &instrument) -> bool {
    assert(instrument.order_state == OrderState::IDLE);
    assert(instrument.order_id == 0);
    instrument.order_state = OrderState::CREATE;
    instrument.order_id = ++max_order_id_;
    auto quantity = 1.0;                                                                 // XXX FIXME TODO compute quantity
    auto price = utils::price_from_side(instrument.top_of_book(), utils::invert(side));  // note! aggress liquidity on other side
    auto create_order = CreateOrder{
        .account = instrument.account,
        .order_id = instrument.order_id,
        .exchange = instrument.exchange,
        .symbol = instrument.symbol,
        .side = side,
        .position_effect = position_effect_,
        .margin_mode = margin_mode_,
        .max_show_quantity = NaN,
        .order_type = OrderType::LIMIT,
        .time_in_force = time_in_force_,
        .execution_instructions = {},
        .request_template = {},
        .quantity = quantity,
        .price = price,
        .stop_price = NaN,
        .routing_id = {},
        .strategy_id = strategy_id_,
    };
    log::debug("[{}] create_order={}"sv, instrument.source, create_order);
    try {
      dispatcher_.send(create_order, instrument.source);
      // XXX FIXME TODO record timestamp (or something like that) so we can manage rate-limitations
    } catch (NotReady &) {
      instrument.reset();
      return false;
    }
    return true;
  };
  if (can_trade(side, lhs) && helper(side, lhs)) {
    // note! only try second leg if first has succeeded
    if (!helper(utils::invert(side), rhs)) {
      // note! we should try-cancel first leg if second has failed
      // XXX FIXME TODO cancel first leg
    }
  }
}

bool Simple::can_trade(Side side, Instrument &instrument) const {
  auto position = instrument.current_position();
  auto position_0 = position;  // XXX FIXME TODO scale to base of instrument[0]
  switch (side) {
    using enum Side;
    case UNDEFINED:
      assert(false);
      break;
    case BUY:
      return utils::compare(position_0, max_position_0_) < 0;
    case SELL:
      return utils::compare(position_0, min_position_0_) > 0;
  }
  return false;
}

template <typename T>
bool Simple::is_my_order(Event<T> const &event) const {
  auto &[message_info, value] = event;
  return !strategy_id_ || strategy_id_ == value.strategy_id;
}

template <typename T>
void Simple::check(Event<T> const &event) {
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

// XXX FIXME TODO proper (for now, just testing simulator support)
void Simple::publish_statistics(Instrument &instrument) {
  auto &top_of_book = instrument.top_of_book();
  std::array<Measurement, 4> measurements{{
      {
          .name = "bp"sv,
          .value = top_of_book.bid_price,
      },
      {
          .name = "bq"sv,
          .value = top_of_book.bid_quantity,
      },
      {
          .name = "ap"sv,
          .value = top_of_book.ask_price,
      },
      {
          .name = "aq"sv,
          .value = top_of_book.ask_quantity,
      },
  }};
  auto custom_metrics = CustomMetrics{
      .label = "top_of_book"sv,
      .account = instrument.account,
      .exchange = instrument.exchange,
      .symbol = instrument.symbol,
      .measurements = measurements,
      .update_type = UpdateType::INCREMENTAL,
  };
  dispatcher_.send(custom_metrics, publish_source_);
  //
  std::array<MatrixKey, 2> headers{{
      "0"sv,
      "1"sv,
  }};
  std::array<double, 4> data{{
      0.0,
      1.1,
      2.2,
      3.3,
  }};
  auto custom_matrix = CustomMatrix{
      .label = "spreads"sv,
      .account = {},
      .exchange = {},
      .symbol = {},
      .rows = headers,
      .columns = headers,
      .data = data,
      .update_type = UpdateType::INCREMENTAL,
      .version = {},
  };
  dispatcher_.send(custom_matrix, publish_source_);
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
