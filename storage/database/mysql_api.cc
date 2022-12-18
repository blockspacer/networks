#include "api.h"
#include "mysql_storage.h"

extern "C" storage::IStorage* CreateStorage(const char* storage_config) {
  return storage::database::MySqlStorage::Create(storage::database::LoadFromFile(storage_config));
}

extern "C" void DestroyStorage(storage::IStorage* storage) {
  delete static_cast<storage::database::MySqlStorage*>(storage);
}
