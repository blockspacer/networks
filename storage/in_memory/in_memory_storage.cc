#include "in_memory_storage.h"

#include "core/datetime.h"

void storage::InMemoryStorage::Store(const proto::Message& message) {
  auto copy = message;
  copy.set_message_uid(core::atomics::GetAndIncrement(counter_));
  for (const auto& to : message.to()) {
    storage_[to].insert(copy);
  }
}

std::vector<proto::Message> storage::InMemoryStorage::Load(const std::vector<std::string>& possible_addressees) {
  std::vector<proto::Message> result;
  proto::Message pivot;
  pivot.set_send_ts(absl::ToUnixSeconds(absl::Now()));
  for (const auto& t : possible_addressees) {
    const auto it = storage_.find(t);
    if (it != storage_.end()) {
      result.insert(result.end(), it->second.begin(), it->second.upper_bound(pivot));
    }
  }
  return result;
}

std::vector<proto::Message> storage::InMemoryStorage::LoadSended(const std::string& user) {
  std::vector<proto::Message> result;
  for (const auto& to : storage_) {
    for (const auto& message : to.second) {
      if (message.from() == user) {
        result.push_back(message);
      }
    }
  }
  return result;
}
