#include "api.h"
#include "in_memory_storage.h"

#include "core/macro.h"

extern "C" storage::IStorage* CreateStorage(const char* storage_config) {
  core_ignore_result(storage_config);
  return new storage::InMemoryStorage();
}

extern "C" void DestroyStorage(storage::IStorage* storage) { delete static_cast<storage::InMemoryStorage*>(storage); }
