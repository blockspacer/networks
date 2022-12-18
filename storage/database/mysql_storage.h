#pragma once

#include "config.h"

#include "core/tls.h"
#include "storage/storage.h"

#include "mysql++.h"

namespace storage::database {

class MySqlStorage final : public IStorage {
 public:
  static MySqlStorage* Create(const database::Config& config);

  void Store(const proto::Message& message) override;

  std::vector<proto::Message> Load(const std::vector<std::string>& possible_addressees) override;

  std::vector<proto::Message> LoadSended(const std::string& user) override;

 private:
  MySqlStorage(const database::Config& config);

 private:
  core_thread(mysqlpp::Connection) connection_;
  std::string table_;
};

}  // namespace storage::database