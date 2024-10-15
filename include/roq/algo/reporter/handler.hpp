/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <string_view>

#include "roq/client/collector.hpp"

#include "roq/algo/reporter/output_type.hpp"

namespace roq {
namespace algo {
namespace reporter {

struct Handler : public client::Collector {
  virtual void print(OutputType = {}, std::string_view const &label = {}) const = 0;
  virtual void write(std::string_view const &path, OutputType = {}, std::string_view const &label = {}) const = 0;
};

}  // namespace reporter
}  // namespace algo
}  // namespace roq
