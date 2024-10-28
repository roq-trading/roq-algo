/* Copyright (c) 2017-2024, Hans Erik Thrane */

#pragma once

#include <memory>
#include <string_view>

#include "roq/client/collector.hpp"

#include "roq/algo/reporter/output_type.hpp"
#include "roq/algo/reporter/type.hpp"

namespace roq {
namespace algo {

struct Reporter : public client::Collector {
  static std::unique_ptr<Reporter> create(reporter::Type = {});

  virtual void print(reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
  virtual void write(std::string_view const &path, reporter::OutputType = {}, std::string_view const &label = {}) const = 0;
};

}  // namespace algo
}  // namespace roq
