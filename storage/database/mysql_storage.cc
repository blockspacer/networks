#include "mysql_storage.h"

#include "core/datetime.h"
#include "core/exception.h"

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

storage::database::MySqlStorage* storage::database::MySqlStorage::Create(const storage::database::Config& config) {
  MySqlStorage* r = nullptr;
  try {
    r = new MySqlStorage(config);
  } catch (const mysqlpp::Exception& e) {
    core_throw core::Exception() << e.what();
  }
  return r;
}

storage::database::MySqlStorage::MySqlStorage(const database::Config& config)
    : connection_(mysqlpp::Connection(config.schema.c_str(), config.host.c_str(), config.username.c_str(),
                                      config.password.c_str(), config.port))
    , table_(config.table) {}

void storage::database::MySqlStorage::Store(const proto::Message& message) {
  auto& connection = core::TlsRef(connection_);
  auto query = connection.query();
  query << "INSERT INTO " << table_ << "(sender, receiver, all_receivers, send_time, message, reply) "
        << "VALUES ";
  const auto to_all = absl::StrJoin(message.to(), ";");
  for (size_t i = 0; i < message.to().size(); ++i) {
    query << "('" << message.from() << "', '" << message.to()[i] << "', '" << to_all << "', " << message.send_ts()
          << ", '" << message.message() << "', ";
    if (message.reply_size() == 1) {
      query << "'" << message.reply(0) << "')";
    } else {
      query << mysqlpp::null_str << ")";
    }
    if (i < message.to().size() - 1) {
      query << ", ";
    }
  }
  query << ";";
  mysqlpp::Transaction txn(connection);
  try {
    if (query.execute()) {
      txn.commit();
    }
  } catch (const mysqlpp::BadQuery& e) {
    core_throw core::Exception() << e.what() << "\n Query was: " << query.str();
  } catch (const mysqlpp::BadConversion& e) {
    core_throw core::Exception() << e.what() << "\n Query was: " << query.str();
  } catch (const mysqlpp::Exception& e) {
    core_throw core::Exception() << e.what() << "\n Query was: " << query.str();
  }
}

std::vector<proto::Message> storage::database::MySqlStorage::Load(const std::vector<std::string>& possible_addressees) {
  auto& connection = core::TlsRef(connection_);
  std::vector<proto::Message> result;
  auto now = absl::ToUnixSeconds(absl::Now());
  try {
    for (const auto& to : possible_addressees) {
      auto query = connection.query();
      query << "SELECT id, sender, all_receivers, send_time, message, reply "
            << "FROM " << table_ << " WHERE receiver = '" << to << "' AND send_time <= " << now << ";";
      auto res = query.store();
      for (size_t i = 0; i < res.num_rows(); ++i) {
        proto::Message message;
        message.set_message_uid(res[i]["id"]);
        message.set_from(res[i]["sender"]);
        auto split_to = absl::StrSplit(std::string(res[i]["all_receivers"]), ';');
        *message.mutable_to() = {split_to.begin(), split_to.end()};
        message.set_send_ts(res[i]["send_time"]);
        message.set_message(res[i]["message"]);
        const auto& reply = res[i]["reply"];
        if (!reply.is_null()) {
          message.add_reply(reply);
        }
        result.push_back(std::move(message));
      }
    }
  } catch (const mysqlpp::BadQuery& e) {
    core_throw core::Exception() << e.what();
  } catch (const mysqlpp::BadConversion& e) {
    core_throw core::Exception() << e.what();
  } catch (const mysqlpp::Exception& e) {
    core_throw core::Exception() << e.what();
  }
  return result;
}

std::vector<proto::Message> storage::database::MySqlStorage::LoadSended(const std::string& user) {
  auto& connection = core::TlsRef(connection_);
  std::vector<proto::Message> result;
  auto now = absl::ToUnixSeconds(absl::Now());
  try {
    auto query = connection.query();
    query << "SELECT id, sender, all_receivers, send_time, message, reply "
          << "FROM " << table_ << " WHERE sender = '" << user << "' AND send_time <= " << now << ";";
    auto res = query.store();
    for (size_t i = 0; i < res.num_rows(); ++i) {
      proto::Message message;
      message.set_message_uid(res[i]["id"]);
      message.set_from(res[i]["sender"]);
      auto split_to = absl::StrSplit(std::string(res[i]["all_receivers"]), ';');
      *message.mutable_to() = {split_to.begin(), split_to.end()};
      message.set_send_ts(res[i]["send_time"]);
      message.set_message(res[i]["message"]);
      const auto& reply = res[i]["reply"];
      if (!reply.is_null()) {
        message.add_reply(reply);
      }
      result.push_back(std::move(message));
    }
  } catch (const mysqlpp::BadQuery& e) {
    core_throw core::Exception() << e.what();
  } catch (const mysqlpp::BadConversion& e) {
    core_throw core::Exception() << e.what();
  } catch (const mysqlpp::Exception& e) {
    core_throw core::Exception() << e.what();
  }
  return result;
}
