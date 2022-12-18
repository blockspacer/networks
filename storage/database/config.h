#pragma once

#include <string>

namespace storage::database {

struct Config {
  std::string host;
  size_t port;

  std::string username;
  std::string password;

  std::string schema;
  std::string table;
};

Config LoadFromFile(const char* filename);

}  // namespace storage::database