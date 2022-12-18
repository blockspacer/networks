#include "config.h"

#include "core/exception.h"
#include "inicpp/inicpp.h"

#include <filesystem>

namespace fs = std::filesystem;

storage::database::Config storage::database::LoadFromFile(const char* filename) {
  if (!fs::exists(filename)) {
    core_throw core::Exception() << "Database config file " << filename << " not found!";
  }
  Config result;
  try {
    auto db_config = inicpp::parser::load_file(filename);

    result.host = db_config["database"]["hostname"].get<std::string>();
    result.port = db_config["database"]["port"].get<size_t>();

    result.username = db_config["database"]["username"].get<std::string>();
    result.password = db_config["database"]["password"].get<std::string>();

    result.schema = db_config["database"]["schema"].get<std::string>();
    result.table = db_config["database"]["table"].get<std::string>();
  } catch (const std::exception& e) {
    core_throw core::Exception() << e.what();
  }
  return result;
}