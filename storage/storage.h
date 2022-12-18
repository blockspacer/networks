#pragma once

#include "proto/message.pb.h"

#include <string_view>
#include <vector>

namespace storage {

struct IStorage {
  enum class LockType { kNone, kSpinLock, kMutex };

  virtual ~IStorage() noexcept = default;

  virtual void Store(const proto::Message& message) = 0;

  virtual std::vector<proto::Message> Load(const std::vector<std::string>& possible_addressees) = 0;

  virtual std::vector<proto::Message> LoadSended(const std::string& user) = 0;

  [[nodiscard]] virtual LockType ProtectStorageBy() const noexcept { return LockType::kNone; }
};

}  // namespace storage
