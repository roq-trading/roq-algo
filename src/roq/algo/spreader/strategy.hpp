/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/client.hpp"

#include "roq/utils/container.hpp"

#include "roq/algo/spreader/instrument.hpp"
#include "roq/algo/spreader/settings.hpp"
#include "roq/algo/spreader/shared.hpp"

namespace roq {
namespace algo {
namespace spreader {

struct Strategy final : public client::Handler {
  Strategy(client::Dispatcher &, Settings const &);

  Strategy(Strategy &&) = default;
  Strategy(Strategy const &) = delete;

 protected:
  void operator()(Event<Timer> const &) override;
  void operator()(Event<Connected> const &) override;
  void operator()(Event<Disconnected> const &) override;
  void operator()(Event<DownloadBegin> const &) override;
  void operator()(Event<DownloadEnd> const &) override;
  void operator()(Event<Ready> const &) override;
  void operator()(Event<ReferenceData> const &) override;
  void operator()(Event<MarketStatus> const &) override;
  void operator()(Event<MarketByPriceUpdate> const &) override;
  void operator()(Event<OrderAck> const &) override;
  void operator()(Event<OrderUpdate> const &) override;
  void operator()(Event<TradeUpdate> const &) override;
  void operator()(Event<PositionUpdate> const &) override;

  template <typename T>
  bool dispatch(Event<T> const &);

  void update();

  void refresh();

  template <typename T>
  void dispatch_2(Event<T> const &);

 private:
  Shared shared_;
  bool ready_ = {};
  roq::utils::unordered_map<std::string, Instrument> instruments_;
};

}  // namespace spreader
}  // namespace algo
}  // namespace roq
