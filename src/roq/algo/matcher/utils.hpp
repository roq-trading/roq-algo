/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/api.hpp"

#include "roq/utils/traits.hpp"

// Event<T>
// 1 argument => must have
// 1 argument => with default
// 2 arguments => fallback must have

namespace roq {
namespace algo {
namespace matcher {

template <typename T>
inline RequestType get_request_type(T const &) {
  using value_type = std::remove_cvref<T>::type;
  if constexpr (std::is_same<value_type, CreateOrder>::value) {
    return RequestType::CREATE_ORDER;
  } else if constexpr (std::is_same<value_type, ModifyOrder>::value) {
    return RequestType::MODIFY_ORDER;
  } else if constexpr (std::is_same<value_type, CancelOrder>::value) {
    return RequestType::CANCEL_ORDER;
  } else {
    static_assert(utils::always_false<T>, "not supported for this type");
  }
}

template <typename T>
inline std::string_view get_exchange(T const &value) {
  constexpr bool has_exchange = requires(T const &t) { t.exchange; };
  if constexpr (has_exchange) {
    return value.exchange;
  } else {
    return {};
  }
}

template <typename T>
inline std::string_view get_symbol(T const &value) {
  constexpr bool has_symbol = requires(T const &t) { t.symbol; };
  if constexpr (has_symbol) {
    return value.symbol;
  } else {
    return {};
  }
}

template <typename T>
inline Side get_side(T const &value) {
  constexpr bool has_side = requires(T const &t) { t.side; };
  if constexpr (has_side) {
    return value.side;
  } else {
    return {};
  }
}

template <typename T>
inline PositionEffect get_position_effect(T const &value) {
  constexpr bool has_position_effect = requires(T const &t) { t.position_effect; };
  if constexpr (has_position_effect) {
    return value.position_effect;
  } else {
    return {};
  }
}

template <typename T>
inline MarginMode get_margin_mode(T const &value) {
  constexpr bool has_margin_mode = requires(T const &t) { t.margin_mode; };
  if constexpr (has_margin_mode) {
    return value.margin_mode;
  } else {
    return {};
  }
}

template <typename T>
inline double get_quantity(T const &value) {
  constexpr bool has_quantity = requires(T const &t) { t.quantity; };
  if constexpr (has_quantity) {
    return value.quantity;
  } else {
    return NaN;
  }
}

template <typename T>
inline double get_price(T const &value) {
  constexpr bool has_price = requires(T const &t) { t.price; };
  if constexpr (has_price) {
    return value.price;
  } else {
    return NaN;
  }
}

template <typename T>
inline double get_stop_price(T const &value) {
  constexpr bool has_stop_price = requires(T const &t) { t.stop_price; };
  if constexpr (has_stop_price) {
    return value.stop_price;
  } else {
    return NaN;
  }
}

template <typename T>
inline uint32_t get_version(T const &value) {
  constexpr bool has_version = requires(T const &t) { t.version; };
  if constexpr (has_version) {
    return value.version;
  }
  return {};
};

template <typename T>
inline uint64_t get_order_id(T const &value) {
  constexpr bool has_order_id = requires(T const &t) { t.order_id; };
  if constexpr (has_order_id) {
    return value.order_id;
  } else {
    static_assert(utils::always_false<T>, "not supported for this type");
  }
}

template <typename T>
OrderAck create_order_ack(T const &value, Error error, RequestStatus request_status) {
  auto get_request_status = [&]() {
    if (request_status != RequestStatus{})
      return request_status;
    if (error != Error{})
      return RequestStatus::REJECTED;
    return RequestStatus::ACCEPTED;
  };
  auto get_text = [&]() -> std::string_view {
    if (error != Error{})
      return magic_enum::enum_name(error);
    return {};
  };
  return {
      .account = value.account,
      .order_id = value.order_id,
      .exchange = get_exchange(value),
      .symbol = get_symbol(value),
      .side = get_side(value),
      .position_effect = get_position_effect(value),
      .margin_mode = get_margin_mode(value),
      .request_type = get_request_type(value),
      .origin = Origin::EXCHANGE,
      .request_status = get_request_status(),
      .error = error,
      .text = get_text(),
      .request_id = {},  // XXX framework ???
      .external_account = {},
      .external_order_id = {},
      .client_order_id = {},  // XXX FIXME
      .quantity = get_quantity(value),
      .price = get_price(value),
      .stop_price = get_stop_price(value),
      .routing_id = {},               // XXX framework
      .version = get_version(value),  // XXX framework ???
      .risk_exposure = NaN,           // XXX framework
      .risk_exposure_change = NaN,    // XXX framework
      .traded_quantity = NaN,
      .round_trip_latency = {},  // XXX framework
      .user = {},                // XXX framework
  };
}

template <typename T, typename Order>
inline std::string_view get_exchange(T const &value, Order const &order) {
  constexpr bool has_exchange = requires(T const &t) { t.exchange; };
  if constexpr (has_exchange) {
    return value.exchange;
  } else {
    return order.exchange;
  }
}

template <typename T, typename Order>
inline std::string_view get_symbol(T const &value, Order const &order) {
  constexpr bool has_symbol = requires(T const &t) { t.symbol; };
  if constexpr (has_symbol) {
    return value.symbol;
  } else {
    return order.symbol;
  }
}

template <typename T, typename Order>
inline Side get_side(T const &value, Order const &order) {
  constexpr bool has_side = requires(T const &t) { t.side; };
  if constexpr (has_side) {
    return value.side;
  } else {
    return order.side;
  }
}

template <typename T, typename Order>
inline PositionEffect get_position_effect(T const &value, Order const &order) {
  constexpr bool has_position_effect = requires(T const &t) { t.position_effect; };
  if constexpr (has_position_effect) {
    return value.position_effect;
  } else {
    return order.position_effect;
  }
}

template <typename T, typename Order>
inline MarginMode get_margin_mode(T const &value, Order const &order) {
  constexpr bool has_margin_mode = requires(T const &t) { t.margin_mode; };
  if constexpr (has_margin_mode) {
    return value.margin_mode;
  } else {
    return order.margin_mode;
  }
}

template <typename T, typename Order>
inline double get_quantity(T const &value, Order const &order) {
  constexpr bool has_quantity = requires(T const &t) { t.quantity; };
  if constexpr (has_quantity) {
    return value.quantity;
  } else {
    return order.quantity;
  }
}

template <typename T, typename Order>
inline double get_price(T const &value, Order const &order) {
  constexpr bool has_price = requires(T const &t) { t.price; };
  if constexpr (has_price) {
    return value.price;
  } else {
    return order.price;
  }
}

template <typename T, typename Order>
inline double get_stop_price(T const &value, Order const &order) {
  constexpr bool has_stop_price = requires(T const &t) { t.stop_price; };
  if constexpr (has_stop_price) {
    return value.stop_price;
  } else {
    return order.stop_price;
  }
}

template <typename T, typename Order>
inline uint32_t get_version(T const &value, Order const &order) {
  constexpr bool has_version = requires(T const &t) { t.version; };
  if constexpr (has_version) {
    if (value.version)
      return value.version;
  }
  return order.max_request_version;
};

template <typename T, typename Order>
OrderAck create_order_ack(T const &value, Order const &order, Error error, RequestStatus request_status) {
  auto get_request_status = [&]() {
    if (request_status != RequestStatus{})
      return request_status;
    if (error != Error{})
      return RequestStatus::REJECTED;
    return RequestStatus::ACCEPTED;
  };
  auto get_text = [&]() -> std::string_view {
    if (error != Error{})
      return magic_enum::enum_name(error);
    return {};
  };
  return {
      .account = value.account,
      .order_id = value.order_id,
      .exchange = get_exchange(value, order),
      .symbol = get_symbol(value, order),
      .side = get_side(value, order),
      .position_effect = order.position_effect,
      .margin_mode = order.margin_mode,
      .request_type = get_request_type(value),
      .origin = Origin::EXCHANGE,
      .request_status = get_request_status(),
      .error = error,
      .text = get_text(),
      .request_id = {},  // XXX framework ???
      .external_account = {},
      .external_order_id = {},
      .client_order_id = {},  // XXX FIXME
      .quantity = get_quantity(value, order),
      .price = get_price(value, order),
      .stop_price = get_stop_price(value, order),
      .routing_id = {},                      // XXX framework
      .version = get_version(value, order),  // XXX framework ???
      .risk_exposure = NaN,                  // XXX framework
      .risk_exposure_change = NaN,           // XXX framework
      .traded_quantity = order.traded_quantity,
      .round_trip_latency = {},  // XXX framework
      .user = {},                // XXX framework
  };
}

}  // namespace matcher
}  // namespace algo
}  // namespace roq
