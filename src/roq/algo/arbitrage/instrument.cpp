/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/arbitrage/instrument.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace arbitrage {

// === IMPLEMENTATION ===

Instrument::Instrument(algo::Instrument const &item, MarketDataSource market_data_source)
    : source{item.source}, exchange{item.exchange}, symbol{item.symbol}, account{item.account}, market_{exchange, symbol, market_data_source} {
}

}  // namespace arbitrage
}  // namespace algo
}  // namespace roq
