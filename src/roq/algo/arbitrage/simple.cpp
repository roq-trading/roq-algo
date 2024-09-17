/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/simple.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === HELPERS ===

namespace {}  // namespace

// === IMPLEMENTATION ===

Simple::Simple(strategy::Dispatcher &dispatcher, Cache &cache, std::string_view const &exchange, std::string_view const &symbol, Config const &config)
    : dispatcher_{dispatcher}, cache_{cache}, config_{config} {
}

void Simple::operator()(Event<ReferenceData> const &event) {
  /*
  auto &[message_info, reference_data] = event;
  dispatcher_(event);
  utils::update_max(exchange_time_utc_, reference_data.exchange_time_utc);
  top_of_book_(event);
  (*market_by_price_)(event);
  (*market_by_order_)(event);
  if (utils::update(tick_size_, event.value.tick_size)) {
    auto precision = market::increment_to_precision(tick_size_);
    if (static_cast<std::underlying_type<decltype(precision)>::type>(precision) < static_cast<std::underlying_type<decltype(precision_)>::type>(precision_))
      log::fatal("Unexpected"sv);  // note! we can't support loss of precision
    // XXX FIXME internal prices must be scaled relatively
    precision_ = precision;
  }
  */
}

void Simple::operator()(Event<MarketStatus> const &event) {
  /*
  auto &[message_info, market_status] = event;
  dispatcher_(event);
  utils::update_max(exchange_time_utc_, market_status.exchange_time_utc);
  if (!market_status_(event))
    return;
  */
}

void Simple::operator()(Event<TopOfBook> const &event) {
  /*
  auto &[message_info, top_of_book] = event;
  utils::update_max(exchange_time_utc_, top_of_book.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (potentially overlay own orders here)
  if (!top_of_book_(event))
    return;
  if (config_.source != Source::TOP_OF_BOOK)
    return;
  if (std::isnan(tick_size_))  // note! haven't received any reference data
    return;
  Event event_2{message_info, top_of_book_.layer};
  (*this)(event_2);
  */
}

void Simple::operator()(Event<MarketByPriceUpdate> const &event) {
  /*
  auto &[message_info, market_by_price_update] = event;
  utils::update_max(exchange_time_utc_, market_by_price_update.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (potentially overlay own orders here)
  (*market_by_price_)(event);
  if (config_.source != Source::MARKET_BY_PRICE)
    return;
  Layer layer;
  (*market_by_price_).extract({&layer, 1}, true);
  Event event_2{message_info, layer};
  (*this)(event_2);
  */
}

void Simple::operator()(Event<MarketByOrderUpdate> const &event) {
  /*
  auto &[message_info, market_by_order_update] = event;
  utils::update_max(exchange_time_utc_, market_by_order_update.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (potentially overlay own orders here)
  (*market_by_order_)(event);
  if (config_.source != Source::MARKET_BY_ORDER)
    return;
  Layer layer;
  (*market_by_order_).extract_2({&layer, 1});
  Event event_2{message_info, layer};
  (*this)(event_2);
  */
}

void Simple::operator()(Event<TradeSummary> const &event) {
  /*
  auto &[message_info, trade_summary] = event;
  utils::update_max(exchange_time_utc_, trade_summary.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (potentially overlay own fills here)
  */
}

void Simple::operator()(Event<StatisticsUpdate> const &event) {
  /*
  auto &[message_info, statistics_update] = event;
  utils::update_max(exchange_time_utc_, statistics_update.exchange_time_utc);
  dispatcher_(event);  // note! dispatch market data (potentially overlay own fills here)
  */
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
