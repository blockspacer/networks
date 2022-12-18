#pragma once

#include "core/atomic.h"
#include "storage/storage.h"

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"

namespace storage {

class InMemoryStorage final : public IStorage {
 public:
  void Store(const proto::Message& message) override;

  std::vector<proto::Message> Load(const std::vector<std::string>& possible_addressees) override;

  std::vector<proto::Message> LoadSended(const std::string& user) override;

 private:
  struct MessageComparator {
    inline bool operator()(const proto::Message& l, const proto::Message& r) const noexcept {
      return std::less<>{}(l.send_ts(), r.send_ts());
    }
  };

 private:
  core::Atomic counter_ = 0;
  absl::flat_hash_map<std::string, absl::btree_set<proto::Message, MessageComparator>> storage_;
};

}  // namespace storage