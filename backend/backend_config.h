#pragma once

#include "logging.h"

#include "storage/config.h"

#include <string>
#include <string_view>

namespace backend {

struct Config {
  size_t threads_num;

  std::string pid_file;
  std::string host;
  uint64_t port;

  storage::Config storage_config;
  LoggerConfig log_config;
};

Config LoadFromFile(std::string_view file);

}  // namespace backend
