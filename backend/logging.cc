#include "logging.h"

#include "spdlog/sinks/rotating_file_sink.h"

#include "core/exception.h"

void backend::InitializeLogger(const LoggerConfig& config) {
  try {
    auto logger =
        spdlog::rotating_logger_mt("chat_logger", config.log_file, config.max_file_size, config.max_file_count);
    logger->set_pattern("[%n] (%H:%M:%S:%e %z) {%t} %l: %v");
    spdlog::flush_every(std::chrono::seconds(5));
  } catch (const spdlog::spdlog_ex& ex) {
    core_throw core::Exception() << ex.what();
  }
}