#pragma once

#include "config.h"
#include "core/tls.h"
#include "storage/storage.h"

#include "pqxx/pqxx"

namespace storage::database {

namespace detail {

class ConnectionWrapper {
 public:
  ConnectionWrapper(const database::Config& config);

  inline auto& operator()() { return connection_; }

  inline const auto& operator()() const { return connection_; }

 private:
  pqxx::connection connection_;
};

}  // namespace detail

class PostgreSqlStorage final : public IStorage {
 public:
  static PostgreSqlStorage* Create(const database::Config& config);

  void Store(const proto::Message& message) override;

  std::vector<proto::Message> Load(const std::vector<std::string>& possible_addressees) override;

  std::vector<proto::Message> LoadSended(const std::string& user) override;

 private:
  PostgreSqlStorage(const database::Config& config);

 private:
  core_thread(detail::ConnectionWrapper) connection_;
};

}  // namespace storage::database
