/* Copyright (c) 2017-2024, Hans Erik Thrane */

#include "roq/algo/spreader/strategy.hpp"

#include "roq/logging.hpp"

#include "roq/algo/tools/simple.hpp"

#include "roq/algo/spreader/flags/flags.hpp"

using namespace std::literals;

namespace roq {
namespace algo {
namespace spreader {

Strategy::Strategy(roq::client::Dispatcher &dispatcher, Settings const &settings)
    : dispatcher_{dispatcher}, symbols_{settings.symbols}, update_freq_{settings.test.update_freq} {
}

void Strategy::operator()(roq::Event<roq::Timer> const &event) {
  // note!
  //   ROQ_v is the environment variable controlling the log verbosity level
  //   ROQ_v=5 (or higher) will enable this line, otherwise there will be no logging
  roq::log::info<5>("event={}"sv, event);
}

void Strategy::operator()(roq::Event<roq::Connected> const &) {
  // note!
  //   Always logged (no verbosity level specified)
  roq::log::info("1+2={}"sv, tools::Simple::add(1, 2));
}

void Strategy::operator()(roq::Event<roq::Disconnected> const &) {
}

void Strategy::operator()(roq::Event<roq::DownloadBegin> const &event) {
  auto &[message_info, download_begin] = event;
  roq::log::warn(R"(*** DOWNLOAD NOW IN PROGRESS *** (account="{}"))"sv, download_begin.account);
  // note!
  //   You must not send any requests during a download!
}

void Strategy::operator()(roq::Event<roq::DownloadEnd> const &event) {
  auto &[message_info, download_end] = event;
  roq::log::warn<0>(R"(*** DOWNLOAD HAS COMPLETED *** (account="{}", max_order_id={}))"sv, download_end.account, download_end.max_order_id);
}

void Strategy::operator()(roq::Event<roq::GatewayStatus> const &) {
}

void Strategy::operator()(roq::Event<roq::ReferenceData> const &) {
}

void Strategy::operator()(roq::Event<roq::MarketStatus> const &) {
}

void Strategy::operator()(roq::Event<roq::MarketByPriceUpdate> const &) {
}

void Strategy::operator()(roq::Event<roq::OrderAck> const &) {
}

void Strategy::operator()(roq::Event<roq::OrderUpdate> const &) {
}

void Strategy::operator()(roq::Event<roq::TradeUpdate> const &) {
}

void Strategy::operator()(roq::Event<roq::PositionUpdate> const &) {
}

void Strategy::operator()(roq::Event<roq::FundsUpdate> const &) {
}

}  // namespace spreader
}  // namespace algo
}  // namespace roq
