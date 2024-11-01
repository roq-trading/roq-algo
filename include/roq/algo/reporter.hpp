/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <chrono>
#include <string_view>

#include "roq/variant_type.hpp"

#include "roq/client/collector.hpp"

#include "roq/algo/reporter/output_type.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Reporter : public client::Collector {
  enum class Type {
    INDEX,
    DATA,
  };
  struct Handler {
    virtual void operator()(std::string_view const &name, Type, std::span<std::string_view const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<bool const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<uint8_t const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<uint32_t const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<uint64_t const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<double const> const &) = 0;
    virtual void operator()(std::string_view const &name, Type, std::span<std::chrono::nanoseconds const> const &) = 0;
  };

  virtual std::span<std::string_view const> get_labels() const = 0;

  virtual void dispatch(Handler &, std::string_view const &label) const = 0;

  virtual void print(reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
  virtual void write(std::string_view const &path, reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
};

}  // namespace algo
}  // namespace roq
