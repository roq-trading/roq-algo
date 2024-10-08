/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/reporter/summary.hpp"

#include <cassert>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"

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
        log::info("      order_ack={}"sv, instrument.order_ack);
        log::info("      order_update={}"sv, instrument.order_update);
        log::info("      trade_update:"sv);
        log::info("        fills={}"sv, instrument.trade_update.fills);
      }
    }
  }
}

// collector

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
