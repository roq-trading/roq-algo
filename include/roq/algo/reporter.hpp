/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include "roq/compat.hpp"

#include <memory>
#include <string_view>

#include "roq/client/collector.hpp"

#include "roq/algo/reporter/output_type.hpp"

namespace roq {
namespace algo {

struct ROQ_PUBLIC Reporter : public client::Collector {
  virtual void print(reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
  virtual void write(std::string_view const &path, reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
};

}  // namespace algo
}  // namespace roq
