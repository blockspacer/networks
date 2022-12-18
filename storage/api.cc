#include "api.h"
#include "storage_lock.h"

#include "core/dynlib.h"

namespace dll_api {

using StorageCreate = storage::IStorage* (*)(const char*);
using StorageDestroy = void (*)(storage::IStorage*);

}  // namespace dll_api

namespace {

class StorageFromDll final : public storage::IStorage {
 public:
  StorageFromDll(const storage::Config& config)
      : lock_(type_) {
    dll_.Open(config.storage_dll.data());
    creator_ = (dll_api::StorageCreate)dll_.Sym("CreateStorage");
    destroyer_ = (dll_api::StorageDestroy)dll_.Sym("DestroyStorage");

    storage_ = creator_(config.storage_config.data());
    type_ = storage_->ProtectStorageBy();
  }

  ~StorageFromDll() override {
    if (destroyer_ != nullptr) {
      destroyer_(storage_);
    }
    dll_.Close();
  }

  void Store(const proto::Message& message) override {
    core_with_lock(lock_) { storage_->Store(message); }
  }

  std::vector<proto::Message> Load(const std::vector<std::string>& possible_addressees) override {
    core_with_lock(lock_) { return storage_->Load(possible_addressees); }
  }

  std::vector<proto::Message> LoadSended(const std::string& user) override {
    core_with_lock(lock_) { return storage_->LoadSended(user); }
  }

 private:
  IStorage* storage_ = nullptr;
  dll_api::StorageCreate creator_ = nullptr;
  dll_api::StorageDestroy destroyer_ = nullptr;

  storage::IStorage::LockType type_ = storage::IStorage::LockType::kNone;
  storage::StorageLock lock_;

  core::DynamicLibrary dll_;
};

}  // namespace

std::unique_ptr<storage::IStorage> storage::CreateStorage(const Config& config) {
  return std::make_unique<StorageFromDll>(config);
}