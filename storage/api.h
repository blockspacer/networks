#pragma once

#include "config.h"
#include "storage.h"

#include <memory>

namespace storage {

std::unique_ptr<IStorage> CreateStorage(const Config& config);

}  // namespace storage