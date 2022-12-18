#pragma once

#include "storage/storage.h"

extern "C" storage::IStorage* CreateStorage(const char* storage_config);
extern "C" void DestroyStorage(storage::IStorage* storage);