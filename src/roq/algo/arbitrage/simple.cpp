/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/market/mbp/factory.hpp"

#include "roq/market/mbo/factory.hpp"

#include "roq/algo/utils/common.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === CONSTANTS ===

namespace {
auto const ACCOUNT = "A1"sv;  // XXX FIXME TODO from config
}

// === HELPERS ===

namespace {
auto create_market_by_price(auto &instrument) {
  return market::mbp::Factory::create(instrument.exchange, instrument.symbol);
}

auto create_market_by_order(auto &instrument) {
  return market::mbo::Factory::create(instrument.exchange, instrument.symbol);
}

template <typename R>
auto create_instruments(auto &config) {
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto size = std::size(config.instruments);
  assert(size >= 2);
  if (size < 2)
    log::fatal("Unexpected: len(config.instruments)={}"sv, size);
  for (size_t i = 0; i < size; ++i) {
    auto &item = config.instruments[i];
    Simple::State state;
    state.market_by_price = create_market_by_price(item);
    state.market_by_order = create_market_by_order(item);
    auto instrument = Simple::Instrument{
        .source = item.source,
        .exchange{item.exchange},
        .symbol{item.symbol},
        .state{std::move(state)},
    };
    log::info("[{}] instrument={}"sv, i, instrument);
    result.emplace_back(std::move(instrument));
  }
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
  using lookup_type = std::remove_cvref<decltype(result_type::value_type::lookup)>::type;
  std::vector<lookup_type> tmp;
  tmp.resize(max_source + 1);
  for (size_t i = 0; i < std::size(instruments); ++i) {
    auto &item = instruments[i];
    tmp[item.source][item.exchange][item.symbol] = i;
  }
  for (auto &item : tmp)
    result.emplace_back(std::move(item));
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(strategy::Dispatcher &dispatcher, Config const &config, Cache &cache)
    : dispatcher_{dispatcher}, max_age_{config.max_age}, market_data_type_{utils::to_support_type(config.market_data_source)}, account_{ACCOUNT}, cache_{cache},
      instruments_{create_instruments<decltype(instruments_)>(config)}, sources_{create_sources<decltype(sources_)>(instruments_)} {
  assert(!std::empty(instruments_));
  assert(!std::empty(sources_));
}

void Simple::operator()(Event<Timer> const &) {
  // XXX TODO process delayed order requests
}

void Simple::operator()(Event<Disconnected> const &event) {
  get_all_states(event, [](auto &state) { state = {}; });
}

void Simple::operator()(Event<DownloadBegin> const &) {
}

void Simple::operator()(Event<DownloadEnd> const &event) {
  auto &[message_info, download_end] = event;
  if (download_end.account == account_)
    max_order_id_ = std::max(download_end.max_order_id, max_order_id_);
}

void Simple::operator()(Event<Ready> const &event) {
  get_all_states(event, [](auto &state) { state.ready = true; });
  log::info("max_order_id={}"sv, max_order_id_);
  log::info("*** READY ***"sv);
}

void Simple::operator()(Event<StreamStatus> const &event) {
  auto &[message_info, stream_status] = event;
  auto market_data = [&]() {
    // ...
    // XXX FIXME TODO we need better identification of the primary feed
    if (stream_status.supports.has(market_data_type_) && stream_status.priority == Priority::PRIMARY)
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

// XXX FIXME TODO this is the real update (**NOT** correlated with latency assumptions)
void Simple::operator()(Event<ExternalLatency> const &event) {
  auto &[message_info, external_latency] = event;
  auto &source = sources_[message_info.source];
  assert(external_latency.stream_id > 0);
  source.stream_latency.resize(std::max<size_t>(external_latency.stream_id, std::size(source.stream_latency)));
  source.stream_latency[external_latency.stream_id - 1] = external_latency.latency;  // XXX TODO smooth
  // ...
  get_all_states(event, [&](auto &state) {
    if (state.stream_id == external_latency.stream_id)
      state.latency = external_latency.latency;  // XXX TODO smooth
  });
}

void Simple::operator()(Event<ReferenceData> const &event) {
  auto &[message_info, reference_data] = event;
  auto callback = [&](auto &state) {
    roq::utils::update(state.tick_size, reference_data.tick_size);
    (*state.market_by_price)(event);
    (*state.market_by_order)(event);
  };
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
    if (market_data_type_ == SupportType::TOP_OF_BOOK)
      roq::utils::update(state.best, top_of_book.layer);
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  auto &[message_info, market_by_price_update] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, market_by_price_update.exchange_time_utc);
    (*state.market_by_price)(event);
    if (market_data_type_ == SupportType::MARKET_BY_PRICE)
      (*state.market_by_price).extract({&state.best, 1}, true);
    update();
  };
  get_state(event, callback);
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  auto &[message_info, market_by_order_update] = event;
  auto callback = [&](auto &state) {
    roq::utils::update_max(state.exchange_time_utc, market_by_order_update.exchange_time_utc);
    (*state.market_by_order)(event);
    if (market_data_type_ == SupportType::MARKET_BY_ORDER)
      (*state.market_by_order).extract_2({&state.best, 1});
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
  if (message_info.source < std::size(sources_)) [[likely]] {
    auto &lookup = sources_[message_info.source].lookup;
    for (auto &[exchange, tmp] : lookup)
      for (auto &[symbol, index] : tmp)
        callback(instruments_[index].state);
    return;
  }
  log::fatal(R"(Unexpected: source={})"sv, message_info.source);
}

template <typename T, typename Callback>
bool Simple::get_state(Event<T> const &event, Callback callback) {
  auto &[message_info, value] = event;
  if (message_info.source < std::size(sources_)) [[likely]] {
    auto &lookup = sources_[message_info.source].lookup;
    auto iter_1 = lookup.find(value.exchange);
    if (iter_1 != std::end(lookup)) {
      auto &tmp_2 = (*iter_1).second;
      auto iter_2 = tmp_2.find(value.symbol);
      if (iter_2 != std::end(tmp_2)) {
        callback(instruments_[(*iter_2).second].state);
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
  for (auto &item : instruments_) {
    auto &state = item.state;
    result &= is_active(state.trading_status, state.exchange_time_utc) && has_liquidity(state.best.bid_price, state.best.bid_quantity) &&
              has_liquidity(state.best.ask_price, state.best.ask_quantity);
  }
  return result;
}

void Simple::update() {
  for (size_t i = 0; i < std::size(instruments_); ++i)
    log::debug("[{}] instrument={}"sv, i, instruments_[i]);
  if (!can_trade()) {
    // XXX TODO try-cancel working orders?
    return;
  }
  for (size_t i = 0; i < (std::size(instruments_) - 1); ++i) {
    auto &lhs = instruments_[i].state;
    for (size_t j = i + 1; j < std::size(instruments_); ++j) {
      auto &rhs = instruments_[j].state;
      auto spread_1 = lhs.best.bid_price - rhs.best.ask_price;
      auto spread_2 = rhs.best.bid_price - lhs.best.ask_price;
      log::debug("[{}][{}] {} {}"sv, i, j, spread_1, spread_2);
    }
  }
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
