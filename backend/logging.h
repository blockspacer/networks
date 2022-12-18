#pragma once

#include "spdlog/spdlog.h"

#include <string>

#define chat_server_log(x) spdlog::get("chat_logger")->info((x))

namespace backend {

struct LoggerConfig {
  std::string log_file;
  size_t max_file_size;
  size_t max_file_count;
};

void InitializeLogger(const LoggerConfig& config);

}  // namespace backend