/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/tools/market.hpp"

#include <cassert>

#include "roq/logging.hpp"

#include "roq/utils/update.hpp"

#include "roq/market/utils.hpp"

#include "roq/market/mbp/factory.hpp"

#include "roq/market/mbo/factory.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace tools {

// === HELPERS ===

namespace {
auto create_market_by_price(auto &exchange, auto &symbol) {
  return market::mbp::Factory::create(exchange, symbol);
}

auto create_market_by_order(auto &exchange, auto &symbol) {
  return market::mbo::Factory::create(exchange, symbol);
}

template <typename T>
void update_exchange_time_utc(auto &result, Event<T> const &event) {
  // note! we use max because market data could arrive out of sequence (different sources, streams, etc.)
  utils::update_max(result, event.value.exchange_time_utc);
}
}  // namespace

// === IMPLEMENTATION ===

Market::Market(std::string_view const &exchange, std::string_view const &symbol, MarketDataSource market_data_source)
    : market_data_source_{market_data_source}, market_by_price_{create_market_by_price(exchange, symbol)},
      market_by_order_{create_market_by_order(exchange, symbol)} {
}

bool Market::is_market_active(MessageInfo const &message_info, std::chrono::nanoseconds max_age) const {
  if (market_status_.trading_status != TradingStatus{})
    return market_status_.trading_status == TradingStatus::OPEN;
  // use age of market data update as fallback if exchange doesn't support trading status
  return (message_info.receive_time_utc - exchange_time_utc_) < max_age;
}

std::pair<int64_t, bool> Market::price_to_ticks(double price) const {
  assert(has_tick_size());
  return market::price_to_ticks(price, tick_size_, precision_);
}

void Market::operator()(Event<ReferenceData> const &event) {
  update_exchange_time_utc(exchange_time_utc_, event);
  auto &[message_info, reference_data] = event;
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
  roq::utils::update(multiplier_, reference_data.multiplier);
  roq::utils::update(min_trade_vol_, reference_data.min_trade_vol);
}

void Market::operator()(Event<MarketStatus> const &event) {
  update_exchange_time_utc(exchange_time_utc_, event);
  if (!market_status_(event))
    return;
}

bool Market::operator()(Event<TopOfBook> const &event) {
  update_exchange_time_utc(exchange_time_utc_, event);
  if (!top_of_book_(event))
    return false;
  if (market_data_source_ != MarketDataSource::TOP_OF_BOOK)
    return false;
  best_ = top_of_book_.layer;
  return true;
}

bool Market::operator()(Event<MarketByPriceUpdate> const &event) {
  update_exchange_time_utc(exchange_time_utc_, event);
  (*market_by_price_)(event);
  if (market_data_source_ != MarketDataSource::MARKET_BY_PRICE)
    return false;
  (*market_by_price_).extract({&best_, 1}, true);
  return true;
}

bool Market::operator()(Event<MarketByOrderUpdate> const &event) {
  update_exchange_time_utc(exchange_time_utc_, event);
  (*market_by_order_)(event);
  if (market_data_source_ != MarketDataSource::MARKET_BY_ORDER)
    return false;
  (*market_by_order_).extract_2({&best_, 1});
  return true;
}

}  // namespace tools
}  // namespace algo
}  // namespace roq
