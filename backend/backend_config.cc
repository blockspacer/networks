#include "backend_config.h"

#include "core/exception.h"
#include "inicpp/inicpp.h"

#include <filesystem>

backend::Config backend::LoadFromFile(std::string_view file) {
  if (!std::filesystem::exists(file)) {
    core_throw core::Exception() << "Config file " << file << " does not exist";
  }

  Config result;

  try {
    auto server_config = inicpp::parser::load_file(std::string(file));
    result.threads_num = server_config["server"]["threads"].get<size_t>();
    result.pid_file = std::filesystem::absolute(server_config["server"]["pid"].get<std::string>()).string();
    result.port = server_config["server"]["port"].get<uint64_t>();

    result.storage_config.storage_dll = server_config["storage"]["storage_library"].get<std::string>();
    result.storage_config.storage_config = server_config["storage"]["storage_config"].get<std::string>();

    result.log_config.log_file = server_config["logger"]["log_file"].get<std::string>();
    result.log_config.max_file_size = server_config["logger"]["max_file_size"].get<size_t>();
    result.log_config.max_file_count = server_config["logger"]["max_file_count"].get<size_t>();
  } catch (std::exception& e) {
    core_throw core::Exception() << e.what();
  }

  if (!std::filesystem::exists(result.storage_config.storage_dll)) {
    core_throw core::Exception() << "Storage library `" + result.storage_config.storage_dll + "`doesn't exists";
  }

  if (!std::filesystem::exists(result.storage_config.storage_config)) {
    core_throw core::Exception() << "storage config `" + result.storage_config.storage_config + "` doesn't exists";
  }

  return result;
}
