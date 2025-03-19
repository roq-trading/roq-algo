/* Copyright (c) 2017-2025, Hans Erik Thrane */

#include <catch2/catch_all.hpp>

#include "roq/logging.hpp"

#include "roq/utils/container.hpp"

#include "roq/algo/matcher/factory.hpp"

using namespace std::literals;

using namespace Catch::literals;

using namespace roq;

// === CONSTANTS ===

namespace {
auto const SOURCE_NAME = "deribit"sv;
auto const ACCOUNT = "A1"sv;
auto const EXCHANGE = "deribit"sv;
auto const SYMBOL = "BTC-PERPETUAL"sv;
}  // namespace

// === HELPERS ===

namespace {
struct OrderCache final : public algo::OrderCache {
  void operator=(OrderCache &&) = delete;
  void operator=(OrderCache const &) = delete;

  template <typename T>
  cache::Order &operator()(T const &value) {
    if constexpr (std::is_same<T, CreateOrder>::value) {
      auto iter = orders_.find(value.order_id);
      if (iter != std::end(orders_))
        log::fatal("Unexpected"sv);
      iter = orders_.try_emplace(value.order_id, value).first;
      return (*iter).second;
    } else if constexpr (std::is_same<T, ModifyOrder>::value || std::is_same<T, CancelOrder>::value) {
      auto iter = orders_.find(value.order_id);
      if (iter == std::end(orders_))
        log::fatal("Unexpected"sv);
      return (*iter).second;
    } else {
      static_assert(utils::always_false<T>, "not supported for this type");
    }
  }

  template <typename Callback>
  bool find(uint64_t order_id, Callback callback) {
    auto iter = orders_.find(order_id);
    if (iter == std::end(orders_))
      return false;
    callback((*iter).second);
    return true;
  }

 protected:
  cache::Order *get_order_helper(uint64_t order_id) override {
    auto iter = orders_.find(order_id);
    if (iter != std::end(orders_))
      return &(*iter).second;
    return nullptr;
  }
  uint64_t get_next_trade_id() override { return ++next_trade_id_; }

 private:
  utils::unordered_map<uint64_t, cache::Order> orders_;
  uint64_t next_trade_id_ = {};
};

struct Dispatcher final : public algo::Matcher::Dispatcher {
  void set(std::function<void(OrderUpdate const &)> order_update, std::function<void(TradeUpdate const &)> trade_update) {
    order_update_ = order_update;
    trade_update_ = trade_update;
  }

  void set(
      std::function<void(OrderAck const &)> order_ack,
      std::function<void(OrderUpdate const &)> order_update,
      std::function<void(TradeUpdate const &)> trade_update) {
    order_ack_ = order_ack;
    order_update_ = order_update;
    trade_update_ = trade_update;
  }

  void reset() {
    order_ack_ = {};
    order_update_ = {};
    trade_update_ = {};
  }

  bool empty() const {
    if (!order_ack_ && !order_update_ && !trade_update_)
      return true;
    if (order_ack_)
      log::error("MISSING: order_ack"sv);
    if (order_update_)
      log::error("MISSING: order_update"sv);
    if (trade_update_)
      log::error("MISSING: trade_update"sv);
    return false;
  }

 protected:
  void operator()(Event<ReferenceData> const &) override {}
  void operator()(Event<MarketStatus> const &) override {}
  void operator()(Event<TopOfBook> const &) override {}
  void operator()(Event<MarketByPriceUpdate> const &) override {}
  void operator()(Event<MarketByOrderUpdate> const &) override {}
  void operator()(Event<TradeSummary> const &) override {}
  void operator()(Event<StatisticsUpdate> const &) override {}
  void operator()(Event<OrderAck> const &event) override {
    if (order_ack_) {
      order_ack_(event.value);
      order_ack_ = {};
    } else {
      log::error("UNEXPECTED: order_ack"sv);
    }
  }
  void operator()(Event<OrderUpdate> const &event) override {
    if (order_update_) {
      order_update_(event.value);
      order_update_ = {};
    } else {
      log::error("UNEXPECTED: order_update"sv);
    }
  }
  void operator()(Event<TradeUpdate> const &event) override {
    if (trade_update_) {
      trade_update_(event.value);
      trade_update_ = {};
    } else {
      log::error("UNEXPECTED: trade_update"sv);
    }
  }
  void operator()(Event<MassQuoteAck> const &) override {}
  void operator()(Event<CancelQuotesAck> const &) override {}

 private:
  std::function<void(OrderAck const &)> order_ack_;
  std::function<void(OrderUpdate const &)> order_update_;
  std::function<void(TradeUpdate const &)> trade_update_;
};

struct State2 final {
  Dispatcher &dispatcher;
  OrderCache &order_cache;
  algo::Matcher &matcher;
  std::string_view const source_name;
  std::string_view const account;
  std::string_view const exchange;
  std::string_view const symbol;

  uint64_t seqno = {};
  std::chrono::nanoseconds time = {};
  uint64_t next_order_id = {};
};

struct Helper final {
  explicit Helper(State2 &state) : state_{state} {}

  Helper(State2 &state, std::function<void(OrderUpdate const &)> order_update, std::function<void(TradeUpdate const &)> trade_update) : state_{state} {
    state_.dispatcher.set(order_update, trade_update);
  }

  Helper(
      State2 &state,
      std::function<void(OrderAck const &)> order_ack,
      std::function<void(OrderUpdate const &)> order_update,
      std::function<void(TradeUpdate const &)> trade_update)
      : state_{state} {
    state_.dispatcher.set(order_ack, order_update, trade_update);
  }

  ~Helper() { state_.dispatcher.reset(); }

  void operator=(Helper &&) = delete;
  void operator=(Helper const &) = delete;

  void reference_data(double tick_size, double min_trade_vol) {
    auto reference_data = ReferenceData{
        .stream_id = {},
        .exchange = state_.exchange,
        .symbol = state_.symbol,
        .description = {},
        .security_type = {},
        .cfi_code = {},
        .base_currency = {},
        .quote_currency = {},
        .settlement_currency = {},
        .margin_currency = {},
        .commission_currency = {},
        .tick_size = tick_size,
        .tick_size_steps = {},
        .multiplier = NaN,
        .min_notional = NaN,
        .min_trade_vol = min_trade_vol,
        .max_trade_vol = NaN,
        .trade_vol_step_size = NaN,
        .option_type = {},
        .strike_currency = {},
        .strike_price = NaN,
        .underlying = {},
        .time_zone = {},
        .issue_date = {},
        .settlement_date = {},
        .expiry_datetime = {},
        .expiry_datetime_utc = {},
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
        .discard = false,
    };
    dispatch(reference_data);
  };

  void top_of_book(double bid_price, double bid_quantity, double ask_price, double ask_quantity) {
    auto top_of_book = TopOfBook{
        .stream_id = {},
        .exchange = state_.exchange,
        .symbol = state_.symbol,
        .layer{
            .bid_price = bid_price,
            .bid_quantity = bid_quantity,
            .ask_price = ask_price,
            .ask_quantity = ask_quantity,
        },
        .update_type = UpdateType::INCREMENTAL,
        .exchange_time_utc = {},
        .exchange_sequence = {},
        .sending_time_utc = {},
    };
    dispatch(top_of_book);
  };

  uint64_t create_order(Side side, OrderType order_type, TimeInForce time_in_force, double quantity, double price) {
    auto create_order = CreateOrder{
        .account = state_.account,
        .order_id = ++state_.next_order_id,
        .exchange = state_.exchange,
        .symbol = state_.symbol,
        .side = side,
        .position_effect = {},
        .margin_mode = {},
        .quantity_type = {},
        .max_show_quantity = NaN,
        .order_type = order_type,
        .time_in_force = time_in_force,
        .execution_instructions = {},
        .request_template = {},
        .quantity = quantity,
        .price = price,
        .stop_price = NaN,
        .routing_id = {},
        .strategy_id = {},
    };
    dispatch(create_order);
    return create_order.order_id;
  }

  void modify_order(uint64_t order_id, double quantity, double price) {
    auto modify_order = ModifyOrder{
        .account = state_.account,
        .order_id = order_id,
        .request_template = {},
        .quantity = quantity,
        .price = price,
        .routing_id = {},
        .version = {},
        .conditional_on_version = {},
    };
    dispatch(modify_order);
  };

  void cancel_order(uint64_t order_id) {
    auto cancel_order = CancelOrder{
        .account = state_.account,
        .order_id = order_id,
        .request_template = {},
        .routing_id = {},
        .version = {},
        .conditional_on_version = {},
    };
    dispatch(cancel_order);
  };

 protected:
  template <typename T>
  void dispatch(T const &value) {
    auto source_seqno = ++state_.seqno;
    auto now = ++state_.time;
    auto message_info = MessageInfo{
        .source = {},
        .source_name = state_.source_name,
        .source_session_id = {},
        .source_seqno = source_seqno,
        .receive_time_utc = now,
        .receive_time = now,
        .source_send_time = now,
        .source_receive_time = now,
        .origin_create_time = now,
        .origin_create_time_utc = now,
        .is_last = true,
        .opaque = {},
    };
    Event event{message_info, value};
    if constexpr (std::is_same<T, CreateOrder>::value || std::is_same<T, ModifyOrder>::value || std::is_same<T, CancelOrder>::value) {
      auto &order = state_.order_cache(value);
      state_.matcher(event, order);
    } else {
      state_.matcher(event);
    }
    if (std::empty(state_.dispatcher))
      return;
    FAIL();
  };

 private:
  State2 &state_;
};
}  // namespace

// === IMPLEMENTATION ===

TEST_CASE("algo_matcher_simple", "[algo_matcher]") {
  Dispatcher dispatcher;
  OrderCache order_cache;
  auto config = algo::matcher::Config{
      .exchange = EXCHANGE,
      .symbol = SYMBOL,
      .market_data_source = algo::MarketDataSource::TOP_OF_BOOK,
  };
  auto matcher = algo::matcher::Factory::create(algo::matcher::Type::SIMPLE, dispatcher, order_cache, config);
  auto state = State2{
      .dispatcher = dispatcher,
      .order_cache = order_cache,
      .matcher = *matcher,
      .source_name = SOURCE_NAME,
      .account = ACCOUNT,
      .exchange = EXCHANGE,
      .symbol = SYMBOL,
  };
  // ---
  // t=1
  Helper{state}.reference_data(0.1, 1.0);
  // t=2
  Helper{state}.top_of_book(100.0, 1.0, 101.0, 1.0);
  // t=3
  auto order_id = Helper{
      state,
      [&](auto &order_ack) { CHECK(order_ack.request_status == RequestStatus::ACCEPTED); },
      [&](auto &order_update) {
        CHECK(order_update.order_status == OrderStatus::WORKING);
        CHECK(order_update.remaining_quantity == 1.0_a);
        CHECK(order_update.traded_quantity == 0.0_a);
        CHECK(std::isnan(order_update.average_traded_price));
      },
      {}}.create_order(Side::BUY, OrderType::LIMIT, TimeInForce::GTC, 1.0, 100.0);
  REQUIRE(order_id > 0);
  // ...
  REQUIRE(state.order_cache.find(order_id, [&](auto &order) {
    CHECK(order.order_status == OrderStatus::WORKING);
    CHECK(order.remaining_quantity == 1.0_a);
    CHECK(order.traded_quantity == 0.0_a);
    CHECK(std::isnan(order.average_traded_price));
  }));
  // t=4
  Helper{
      state,
      [&](auto &order_update) {
        CHECK(order_update.order_status == OrderStatus::COMPLETED);
        CHECK(order_update.remaining_quantity == 0.0_a);
        CHECK(order_update.traded_quantity == 1.0_a);
        CHECK(order_update.average_traded_price == 100.0_a);
      },
      [&](auto &trade_update) {
        CHECK(trade_update.side == Side::BUY);
        REQUIRE(!std::empty(trade_update.fills));
        for (auto &item : trade_update.fills) {
          CHECK(item.quantity > 0.0);
          CHECK(item.price == 100.0_a);
        }
      }}
      .top_of_book(99.0, 1.0, 100.0, 1.0);
  REQUIRE(state.order_cache.find(order_id, [&](auto &order) {
    CHECK(order.order_status == OrderStatus::COMPLETED);
    CHECK(order.remaining_quantity == 0.0_a);
    CHECK(order.traded_quantity == 1.0_a);
    CHECK(order.average_traded_price == 100.0_a);
  }));
  // t=5
  Helper{
      state,
      [&](auto &order_ack) {
        CHECK(order_ack.request_status == RequestStatus::REJECTED);
        CHECK(order_ack.error == Error::TOO_LATE_TO_MODIFY_OR_CANCEL);
      },
      {},
      {}}
      .modify_order(order_id, NaN, 101.0);
  // t=6
  Helper{
      state,
      [&](auto &order_ack) {
        CHECK(order_ack.request_status == RequestStatus::REJECTED);
        CHECK(order_ack.error == Error::TOO_LATE_TO_MODIFY_OR_CANCEL);
      },
      {},
      {}}
      .cancel_order(order_id);
  // ---
}
