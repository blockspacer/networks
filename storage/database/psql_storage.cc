#include "psql_storage.h"

#include "core/datetime.h"
#include "core/exception.h"

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

#include <sstream>

static auto ConnectionString(const storage::database::Config& config) {
  std::ostringstream out;
  out << "user=" << config.username << " "
      << "host=" << config.host << " "
      << "port=" << config.port << " "
      << "password=" << config.password << " "
      << "dbname=" << config.schema << " ";
  return out.str();
}

storage::database::detail::ConnectionWrapper::ConnectionWrapper(const database::Config& config)
    : connection_(ConnectionString(config)) {
  std::ostringstream ins, sel, sel_send;

  ins << "INSERT INTO " << config.table << " (sender, receiver, all_receivers, send_time, message, reply) "
      << "VALUES ($1,$2,$3,$4,$5, $6);";

  sel << "SELECT id, sender, all_receivers, send_time, message, reply "
      << "FROM " << config.table << " "
      << "WHERE receiver = $1 AND send_time <= $2;";

  sel_send << "SELECT id, sender, all_receivers, send_time, message, reply "
           << "FROM " << config.table << " "
           << "WHERE sender = $1 AND send_time <= $2;";

  connection_.prepare("insert_query", ins.str());
  connection_.prepare("select_query", sel.str());
  connection_.prepare("select_sended_query", sel_send.str());
}

storage::database::PostgreSqlStorage::PostgreSqlStorage(const storage::database::Config& config)
    : connection_(config) {}

storage::database::PostgreSqlStorage* storage::database::PostgreSqlStorage::Create(
    const storage::database::Config& config) {
  PostgreSqlStorage* r = nullptr;
  try {
    r = new PostgreSqlStorage(config);
  } catch (const pqxx::sql_error& e) {
    core_throw core::Exception() << "SQL error: " << e.what() << "\nQuery was: " << e.query();
  } catch (const std::exception& e) {
    core_throw core::Exception() << e.what();
  }
  return r;
}

void storage::database::PostgreSqlStorage::Store(const proto::Message& message) {
  pqxx::work txn{core::TlsRef(connection_)()};
  try {
    const auto to_all = absl::StrJoin(message.to(), ";");
    for (const auto& to : message.to()) {
      if (message.reply_size() == 1) {
        txn.exec_prepared("insert_query", message.from(), to, to_all, message.send_ts(), message.message(),
                          message.reply(0));
      } else {
        txn.exec_prepared("insert_query", message.from(), to, to_all, message.send_ts(), message.message(), nullptr);
      }
    }
    txn.commit();
  } catch (const pqxx::sql_error& e) {
    txn.abort();
    core_throw core::Exception() << "SQL error: " << e.what() << "\nQuery was: " << e.query();
  } catch (const std::exception& e) {
    txn.abort();
    core_throw core::Exception() << e.what();
  }
}

std::vector<proto::Message> storage::database::PostgreSqlStorage::Load(
    const std::vector<std::string>& possible_addressees) {
  std::vector<proto::Message> result;
  auto now = absl::ToUnixSeconds(absl::Now());
  pqxx::work txn{core::TlsRef(connection_)()};

  try {
    for (const auto& to : possible_addressees) {
      auto res = txn.exec_prepared("select_query", to, now);
      for (const auto& row : res) {
        proto::Message message;
        message.set_message_uid(row[0].get<uint64_t>().value());
        message.set_from(row[1].get<std::string>().value());
        auto splitted_to = absl::StrSplit(row[2].get<std::string>().value(), ';');
        *message.mutable_to() = {splitted_to.begin(), splitted_to.end()};
        message.set_send_ts(row[3].get<uint64_t>().value());
        message.set_message(row[4].get<std::string>().value());
        const auto& reply = row[5].get<std::string>();
        if (reply.has_value()) {
          message.add_reply(reply.value());
        }
        result.push_back(std::move(message));
      }
    }
    txn.commit();
  } catch (const pqxx::sql_error& e) {
    txn.abort();
    core_throw core::Exception() << "SQL error: " << e.what() << "\nQuery was: " << e.query();
  } catch (const std::exception& e) {
    txn.abort();
    core_throw core::Exception() << e.what();
  }
  return result;
}

std::vector<proto::Message> storage::database::PostgreSqlStorage::LoadSended(const std::string& user) {
  std::vector<proto::Message> result;
  auto now = absl::ToUnixSeconds(absl::Now());
  pqxx::work txn{core::TlsRef(connection_)()};

  try {
    auto res = txn.exec_prepared("select_sended_query", user, now);
    for (const auto& row : res) {
      proto::Message message;
      message.set_message_uid(row[0].get<uint64_t>().value());
      message.set_from(row[1].get<std::string>().value());
      auto splitted_to = absl::StrSplit(row[2].get<std::string>().value(), ';');
      *message.mutable_to() = {splitted_to.begin(), splitted_to.end()};
      message.set_send_ts(row[3].get<uint64_t>().value());
      message.set_message(row[4].get<std::string>().value());
      const auto& reply = row[5].get<std::string>();
      if (reply.has_value()) {
        message.add_reply(reply.value());
      }
      result.push_back(std::move(message));
    }
    txn.commit();
  } catch (const pqxx::sql_error& e) {
    txn.abort();
    core_throw core::Exception() << "SQL error: " << e.what() << "\nQuery was: " << e.query();
  } catch (const std::exception& e) {
    txn.abort();
    core_throw core::Exception() << e.what();
  }
  return result;
}
