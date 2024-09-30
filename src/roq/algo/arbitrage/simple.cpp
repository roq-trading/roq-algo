/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/algo/utils/common.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {
template <typename R>
auto create_lookup(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = [&]() {
    size_t result = {};
    for (auto &instrument : config.instruments)
      result = std::max<size_t>(result, instrument.source);
    return result + 1;
  }();
  if (size < 2)
    log::fatal("Unexpected: len(config.instruments)={}"sv, size);
  result.resize(size);
  for (size_t i = 0; i < std::size(config.instruments); ++i) {
    auto &instrument = config.instruments[i];
    log::info("[{}] instrument={}"sv, i, instrument);
    result[instrument.source][instrument.exchange][instrument.symbol] = i;
  }
  return result;
}

template <typename R>
auto create_state(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = std::size(config.instruments);
  if (size < 2)
    log::fatal("Unexpected: len(config.instruments)={}"sv, size);
  result.resize(size);
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(strategy::Dispatcher &dispatcher, Config const &config, Cache &cache)
    : dispatcher_{dispatcher}, lookup_{create_lookup<decltype(lookup_)>(config)}, max_age_{config.max_age},
      support_type_{utils::to_support_type(config.market_data_source)}, cache_{cache}, state_{create_state<decltype(state_)>(config)} {
  assert(!std::empty(lookup_));
  assert(std::size(state_) == std::size(lookup_));
}

void Simple::operator()(Event<Timer> const &) {
  // XXX TODO process delayed order requests
}

void Simple::operator()(Event<Disconnected> const &event) {
  get_all_states(event, [](auto &state) { state = {}; });
}

void Simple::operator()(Event<DownloadBegin> const &) {
}

void Simple::operator()(Event<DownloadEnd> const &) {
}

void Simple::operator()(Event<Ready> const &event) {
  get_all_states(event, [](auto &state) { state.ready = true; });
}

void Simple::operator()(Event<StreamStatus> const &event) {
  auto &[message_info, stream_status] = event;
  auto market_data = [&]() {
    // XXX FIXME TODO we need a clear identification of the primary feed
    if (stream_status.supports.has(support_type_) && stream_status.priority == Priority::PRIMARY)
      return true;
    return false;
  }();
  auto order_management = [&]() {
    if (stream_status.supports.has(SupportType::CREATE_ORDER))
      return true;
    return false;
  }();
  if (market_data || order_management)
    get_all_states(event, [&](auto &state) {
      if (market_data)  // XXX FIXME TODO only market data, for now
        state.stream_id = stream_status.stream_id;
    });
}

// XXX FIXME TODO should be simulated
void Simple::operator()(Event<ExternalLatency> const &event) {
  assert(false);  // not yet implemented...
  auto &[message_info, external_latency] = event;
  get_all_states(event, [&](auto &state) {
    if (state.stream_id == external_latency.stream_id)
      state.latency = external_latency.latency;  // XXX TODO smooth
  });
}

void Simple::operator()(Event<ReferenceData> const &event) {
  auto &[message_info, reference_data] = event;
  auto callback = [&](auto &state) { roq::utils::update(state.tick_size, reference_data.tick_size); };
  get_state(event, callback);
}

void Simple::operator()(Event<MarketStatus> const &event) {
  auto &[message_info, market_status] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, market_status.exchange_time_utc);
    roq::utils::update(state.trading_status, market_status.trading_status);
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<TopOfBook> const &event) {
  auto &[message_info, top_of_book] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, top_of_book.exchange_time_utc);
    if (support_type_ == SupportType::TOP_OF_BOOK)
      roq::utils::update(state.best, top_of_book.layer);
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  auto &[message_info, market_by_price_update] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, market_by_price_update.exchange_time_utc);
    if (support_type_ == SupportType::MARKET_BY_PRICE) {
      // XXX TODO implement
    }
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  auto &[message_info, market_by_order_update] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, market_by_order_update.exchange_time_utc);
    if (support_type_ == SupportType::MARKET_BY_ORDER) {
      // XXX TODO implement
    }
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<TradeSummary> const &event) {
  auto &[message_info, trade_summary] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, trade_summary.exchange_time_utc);
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<OrderAck> const &event, cache::Order const &) {
  auto &[message_info, order_ack] = event;
  // auto &state = get_state(event);
}

void Simple::operator()(Event<OrderUpdate> const &event, cache::Order const &) {
  auto &[message_info, order_update] = event;
  // auto &state = get_state(event);
}

void Simple::operator()(Event<PositionUpdate> const &event) {
  auto &[message_info, position_update] = event;
  assert(false);  // not yet implemented...
  log::fatal("DEBUG {}"sv, position_update);
  // auto &state = get_state(event);
}

// utils

template <typename Callback>
void Simple::get_all_states(MessageInfo const &message_info, Callback callback) {
  if (message_info.source < std::size(lookup_)) [[likely]] {
    auto &tmp_1 = lookup_[message_info.source];
    for (auto &[exchange, tmp_2] : tmp_1)
      for (auto &[symbol, index] : tmp_2)
        callback(state_[index]);
    return;
  }
  log::fatal(R"(Unexpected: source={})"sv, message_info.source);
}

template <typename T, typename Callback>
bool Simple::get_state(Event<T> const &event, Callback callback) {
  auto &[message_info, value] = event;
  if (message_info.source < std::size(lookup_)) [[likely]] {
    auto &tmp_1 = lookup_[message_info.source];
    auto iter_1 = tmp_1.find(value.exchange);
    if (iter_1 != std::end(tmp_1)) {
      auto &tmp_2 = (*iter_1).second;
      auto iter_2 = tmp_2.find(value.symbol);
      if (iter_2 != std::end(tmp_2)) {
        callback(state_[(*iter_2).second]);
        return true;
      }
    }
  }
  return false;
}

bool Simple::can_trade() const {
  auto result = true;
  auto is_active = [&](auto trading_status, auto exchange_time_utc) {
    if (trading_status != TradingStatus{})
      return trading_status == TradingStatus::OPEN;
    auto now = clock::get_realtime();
    return (now - exchange_time_utc) < max_age_;
  };
  auto has_liquidity = [](auto price, auto quantity) { return !std::isnan(price) && !std::isnan(quantity) && quantity > 0.0; };
  for (auto &state : state_) {
    result &= is_active(state.trading_status, state.exchange_time_utc) && has_liquidity(state.best.bid_price, state.best.bid_quantity) &&
              has_liquidity(state.best.ask_price, state.best.ask_quantity);
  }
  return result;
}

void Simple::update() {
  // log::debug("state={}"sv, state);
  if (!can_trade()) {
    // XXX TODO try-cancel working orders?
    return;
  }
  for (size_t i = 0; i < (std::size(state_) - 1); ++i) {
    auto &lhs = state_[i];
    for (size_t j = i + 1; j < std::size(state_); ++j) {
      auto &rhs = state_[j];
      auto spread_1 = lhs.best.bid_price - rhs.best.ask_price;
      auto spread_2 = rhs.best.bid_price - lhs.best.ask_price;
      log::debug("[{}][{}] {} {}"sv, i, j, spread_1, spread_2);
    }
  }
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
