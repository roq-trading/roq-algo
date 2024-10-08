/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/reporter/summary.hpp"

#include <cassert>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace reporter {

// === IMPLEMENTATION ===

// reporter

void Summary::print() const {
  for (size_t source = 0; source < std::size(instruments_); ++source) {
    log::info("source={}"sv, source);
    auto &tmp_1 = instruments_[source];
    for (auto &[exchange, tmp_2] : tmp_1) {
      log::info(R"(  exchange="{}")"sv, exchange);
      for (auto &[symbol, instrument] : tmp_2) {
        log::info(R"(    symbol="{}")"sv, symbol);
        log::info("      market_data:"sv);
        log::info("        reference_data={}"sv, instrument.reference_data);
        log::info("        market_status={}"sv, instrument.market_status);
        log::info("        top_of_book={}"sv, instrument.top_of_book);
        log::info("        market_by_price_update={}"sv, instrument.market_by_price_update);
        log::info("        market_by_order_update={}"sv, instrument.market_by_order_update);
        log::info("        trade_summary={}"sv, instrument.trade_summary);
        log::info("        statistics_update={}"sv, instrument.statistics_update);
        log::info("      order_management:"sv);
        log::info("        order_ack={}"sv, instrument.order_ack);
        log::info("        order_update={}"sv, instrument.order_update);
        log::info("        trade_update:"sv);
        log::info("          fills={}"sv, instrument.trade_update.fills);
        log::info("        position_update={}"sv, instrument.position_update);
      }
    }
  }
}

// collector

void Summary::operator()(Event<ReferenceData> const &event) {
  // auto &[message_info, reference_data] = event;
  auto callback = [&](auto &instrument) { ++instrument.reference_data.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<MarketStatus> const &event) {
  // auto &[message_info, market_status] = event;
  auto callback = [&](auto &instrument) { ++instrument.market_status.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<TopOfBook> const &event) {
  // auto &[message_info, top_of_book] = event;
  auto callback = [&](auto &instrument) { ++instrument.top_of_book.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<MarketByPriceUpdate> const &event) {
  // auto &[message_info, market_by_price_update] = event;
  auto callback = [&](auto &instrument) { ++instrument.market_by_price_update.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<MarketByOrderUpdate> const &event) {
  // auto &[message_info, market_by_order_update] = event;
  auto callback = [&](auto &instrument) { ++instrument.market_by_order_update.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<TradeSummary> const &event) {
  // auto &[message_info, trade_summary] = event;
  auto callback = [&](auto &instrument) { ++instrument.trade_summary.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<StatisticsUpdate> const &event) {
  // auto &[message_info, statistics_update] = event;
  auto callback = [&](auto &instrument) { ++instrument.statistics_update.total_count; };
  get_instrument(event, callback);
}

void Summary::operator()(Event<OrderAck> const &event) {
  auto &[message_info, order_ack] = event;
  auto callback = [&](auto &instrument) {
    if (order_ack.origin != Origin::EXCHANGE)
      return;
    if (roq::utils::has_request_completed(order_ack.request_status))
      ++instrument.order_ack.accepted_count;
    if (roq::utils::has_request_failed(order_ack.request_status))
      ++instrument.order_ack.rejected_count;
  };
  get_instrument(event, callback);
}

void Summary::operator()(Event<OrderUpdate> const &event) {
  auto &[message_info, order_update] = event;
  auto callback = [&](auto &instrument) {
    switch (order_update.side) {
      using enum Side;
      case UNDEFINED:
        assert(false);
        break;
      case BUY:
        ++instrument.order_update.buy_count;
        break;
      case SELL:
        ++instrument.order_update.sell_count;
        break;
    }
    ++instrument.order_update.total_count;
  };
  get_instrument(event, callback);
}

void Summary::operator()(Event<TradeUpdate> const &event) {
  auto &[message_info, trade_update] = event;
  auto callback = [&](auto &instrument) {
    for (auto &fill : trade_update.fills) {
      assert(roq::utils::compare(fill.quantity, 0.0) > 0);
      switch (trade_update.side) {
        using enum Side;
        case UNDEFINED:
          assert(false);
          break;
        case BUY:
          ++instrument.trade_update.fills.buy_count;
          instrument.trade_update.fills.buy_volume += fill.quantity;
          break;
        case SELL:
          ++instrument.trade_update.fills.sell_count;
          instrument.trade_update.fills.sell_volume += fill.quantity;
          break;
      }
      ++instrument.trade_update.fills.total_count;
      instrument.trade_update.fills.total_volume += fill.quantity;
    }
  };
  get_instrument(event, callback);
}

void Summary::operator()(Event<PositionUpdate> const &event) {
  auto &[message_info, position_update] = event;
  auto callback = [&](auto &instrument) {
    ++instrument.position_update.total_count;
    auto position = position_update.long_quantity - position_update.short_quantity;
    roq::utils::update_min(instrument.position_update.min_position, position);
    roq::utils::update_max(instrument.position_update.max_position, position);
  };
  get_instrument(event, callback);
}

// utils

template <typename T, typename Callback>
void Summary::get_instrument(Event<T> const &event, Callback callback) {
  auto &[message_info, value] = event;
  instruments_.resize(std::max<size_t>(message_info.source + 1, std::size(instruments_)));
  auto &instrument = instruments_[message_info.source][value.exchange][value.symbol];
  callback(instrument);
}

}  // namespace reporter
}  // namespace algo
}  // namespace roq
