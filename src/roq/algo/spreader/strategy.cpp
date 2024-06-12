/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/strategy.hpp"

#include <magic_enum.hpp>

#include "roq/logging.hpp"

#include "roq/utils/common.hpp"
#include "roq/utils/compare.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace spreader {

// === HELPERS ===

namespace {
auto get_side(auto &value) {
  auto result = magic_enum::enum_cast<Side>(value, magic_enum::case_insensitive);
  if (result.has_value())
    return result.value();
  log::fatal(R"(Unexpected side="{}")"sv, value);
}

template <typename R>
auto create_instruments(auto &settings) {
  // validate
  if (std::size(settings.symbols) < 2)
    log::fatal("Expected at least 2 symbols"sv);
  if (std::size(settings.params) != std::size(settings.symbols))
    log::fatal("Length of parameters should match length of symbols"sv);
  if (std::isnan(settings.quantity))
    log::fatal("Expected quantity"sv);
  // process
  using result_type = std::remove_cvref<R>::type;
  result_type result;
  auto side = get_side(settings.side);
  for (size_t i = 0; i < std::size(settings.symbols); ++i) {
    auto &symbol = settings.symbols[i];
    auto side_2 = i ? utils::invert(side) : side;
    auto quantity = [&]() {
      if (i == 0)
        return settings.quantity;
      auto param = settings.params[i];
      if (std::isnan(param) || utils::is_zero(param))
        log::fatal("Parameter must be non-zero"sv);
      return settings.quantity / param;
    }();
    result.try_emplace(symbol, settings.exchange, symbol, side_2, quantity);
  }
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
  for (auto &[_, instrument] : instruments_)
    instrument.clear();
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
  if (dispatch(event))
    update();
}

void Strategy::operator()(Event<MarketStatus> const &event) {
  if (dispatch(event))
    update();
}

void Strategy::operator()(Event<MarketByPriceUpdate> const &event) {
  if (dispatch(event))
    update();
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
bool Strategy::dispatch(Event<T> const &event) {
  auto iter = instruments_.find(event.value.symbol);
  if (iter == std::end(instruments_)) [[unlikely]]
    log::fatal(R"(Unexpected: symbol="{}")"sv, event.value.symbol);
  return (*iter).second(event);
}

void Strategy::update() {
  log::info("UPDATE"sv);
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
