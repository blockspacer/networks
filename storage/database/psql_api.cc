#include "api.h"
#include "psql_storage.h"

extern "C" storage::IStorage* CreateStorage(const char* storage_config) {
  return storage::database::PostgreSqlStorage::Create(storage::database::LoadFromFile(storage_config));
}

extern "C" void DestroyStorage(storage::IStorage* storage) {
  delete static_cast<storage::database::PostgreSqlStorage*>(storage);
}
