/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/strategy.hpp"

#include <magic_enum.hpp>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"
#include "roq/utils/update.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace spreader {

// === HELPERS ===

namespace {
void validate_settings(auto &settings) {
  if (std::size(settings.symbols) < 2)
    log::fatal("Expected at least 2 symbols"sv);
  if (std::size(settings.params) != std::size(settings.symbols))
    log::fatal("Length of parameters should match length of symbols"sv);
  for (size_t i = 0; i < std::size(settings.params); ++i) {
    auto &param = settings.params[i];
    if (std::isnan(param))
      log::fatal("Parameter can not be NaN"sv);
    if (i && utils::is_zero(param))  // note! base (index 0) has the intercept / zero *is* allowed
      log::fatal("Parameter must be non-zero"sv);
  }
  if (std::isnan(settings.quantity))
    log::fatal("Expected quantity"sv);
}

auto parse_side(auto &value) {
  auto result = magic_enum::enum_cast<Side>(value, magic_enum::case_insensitive);
  if (result.has_value())
    return result.value();
  log::fatal(R"(Unexpected side="{}")"sv, value);
}

template <typename R>
auto create_instruments(auto &settings, auto &shared) {
  validate_settings(settings);
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto side = parse_side(settings.side);
  for (size_t i = 0; i < std::size(settings.symbols); ++i) {
    auto &symbol = settings.symbols[i];
    auto side_2 = i ? utils::invert(side) : side;
    auto quantity = [&]() {
      if (i == 0)
        return settings.quantity;
      return settings.quantity * settings.params[i];
    }();
    auto weight = i ? -settings.params[i] : 1.0;
    result.try_emplace(symbol, shared, settings.exchange, symbol, side_2, quantity, weight);
  }
  return result;
}
}  // namespace

// === IMPLEMENTATION ===

Strategy::Strategy(client::Dispatcher &dispatcher, Settings const &settings)
    : shared_{dispatcher, settings}, instruments_{create_instruments<decltype(instruments_)>(settings, shared_)} {
}

void Strategy::operator()(Event<Timer> const &) {
}

void Strategy::operator()(Event<Connected> const &) {
  log::info("*** CONNECTED ***"sv);
}

void Strategy::operator()(Event<Disconnected> const &) {
  log::warn("*** DISCONNECTED ***"sv);
  ready_ = false;
  for (auto &[_, instrument] : instruments_)
    instrument.clear();
  shared_.ready = false;
}

void Strategy::operator()(Event<DownloadBegin> const &event) {
  auto &[message_info, download_begin] = event;
  log::warn(R"(*** DOWNLOAD NOW IN PROGRESS *** (account="{}"))"sv, download_begin.account);
}

void Strategy::operator()(Event<DownloadEnd> const &event) {
  auto &[message_info, download_end] = event;
  log::warn(R"(*** DOWNLOAD HAS COMPLETED *** (account="{}", max_order_id={}))"sv, download_end.account, download_end.max_order_id);
  if (utils::update_max(shared_.max_order_id, download_end.max_order_id))
    log::info("max_order_id={}"sv, shared_.max_order_id);
}

void Strategy::operator()(Event<Ready> const &) {
  log::info("*** READY ***"sv);
  assert(!ready_);
  ready_ = true;
  update();
}

void Strategy::operator()(Event<ReferenceData> const &event) {
  if (dispatch(event))
    update();
}

void Strategy::operator()(Event<MarketStatus> const &event) {
  if (dispatch(event))
    update();
}

void Strategy::operator()(Event<MarketByPriceUpdate> const &event) {
  if (dispatch(event))
    refresh();
}

void Strategy::operator()(Event<OrderAck> const &event) {
  dispatch_2(event);
}

void Strategy::operator()(Event<OrderUpdate> const &event) {
  dispatch_2(event);
}

void Strategy::operator()(Event<TradeUpdate> const &) {
}

void Strategy::operator()(Event<PositionUpdate> const &) {
}

// market data

template <typename T>
bool Strategy::dispatch(Event<T> const &event) {
  auto iter = instruments_.find(event.value.symbol);
  if (iter == std::end(instruments_)) [[unlikely]]
    log::fatal(R"(Unexpected: symbol="{}")"sv, event.value.symbol);
  return (*iter).second(event);
}

// state change
void Strategy::update() {
  if (!ready_)  // downloading?
    return;
  auto ready = true;
  for (auto &[_, instrument] : instruments_)
    ready &= instrument.ready();
  if (utils::update(shared_.ready, ready)) {
    log::info("DEBUG ready={}"sv, shared_.ready);
    // note! wait for next tick before doing anything
  }
}

// market data change
void Strategy::refresh() {
  if (!shared_.ready)
    return;
  double residual = 0.0;
  for (auto &[_, instrument] : instruments_)
    residual += instrument.value();
  if (std::isnan(residual))
    return;
  log::info("DEBUG residual={}"sv, residual);
  for (auto &[_, instrument] : instruments_)
    instrument.update(residual);
}

// order management

template <typename T>
void Strategy::dispatch_2(Event<T> const &event) {
  auto iter = instruments_.find(event.value.symbol);
  if (iter == std::end(instruments_)) [[unlikely]]
    log::fatal(R"(Unexpected: symbol="{}")"sv, event.value.symbol);
  (*iter).second(event);
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
