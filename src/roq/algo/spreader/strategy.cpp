/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/strategy.hpp"

#include "roq/logging.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace spreader {

// === HELPERS ===

namespace {
template <typename R>
auto create_instruments(auto &settings) {
  // validate
  if (std::size(settings.symbols) < 2)
    log::fatal("Expected at least 2 symbols"sv);
  if (std::size(settings.params) != std::size(settings.symbols))
    log::fatal("Length of parameters should match length of symbols"sv);
  // process
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  for (auto &symbol : settings.symbols)
    result.try_emplace(symbol, settings.exchange, symbol);
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Strategy::Strategy(client::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher_{dispatcher}, instruments_{create_instruments<decltype(instruments_)>(settings)} {
}

void Strategy::operator()(Event<Timer> const &) {
}

void Strategy::operator()(Event<Connected> const &) {
  log::info("*** CONNECTED ***"sv);
}

void Strategy::operator()(Event<Disconnected> const &) {
  log::warn("*** DISCONNECTED ***"sv);
  ready_ = false;
  for (auto &item : instruments_)
    item.second.clear();
}

void Strategy::operator()(Event<DownloadBegin> const &event) {
  auto &[message_info, download_begin] = event;
  log::warn(R"(*** DOWNLOAD NOW IN PROGRESS *** (account="{}"))"sv, download_begin.account);
}

void Strategy::operator()(Event<DownloadEnd> const &event) {
  auto &[message_info, download_end] = event;
  log::warn(R"(*** DOWNLOAD HAS COMPLETED *** (account="{}", max_order_id={}))"sv, download_end.account, download_end.max_order_id);
}

void Strategy::operator()(Event<Ready> const &) {
  log::info("*** READY ***"sv);
  ready_ = true;
}

void Strategy::operator()(Event<ReferenceData> const &event) {
  dispatch(event);
}

void Strategy::operator()(Event<MarketStatus> const &event) {
  dispatch(event);
}

void Strategy::operator()(Event<MarketByPriceUpdate> const &event) {
  dispatch(event);
}

void Strategy::operator()(Event<OrderAck> const &) {
}

void Strategy::operator()(Event<OrderUpdate> const &) {
}

void Strategy::operator()(Event<TradeUpdate> const &) {
}

void Strategy::operator()(Event<PositionUpdate> const &) {
}

template <typename T>
void Strategy::dispatch(Event<T> const &event) {
  auto iter = instruments_.find(event.value.symbol);
  if (iter != std::end(instruments_)) [[likely]] {
    (*iter).second(event);
  } else {
    log::fatal(R"(Unexpected: symbol="{}")"sv, event.value.symbol);
  }
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
