#pragma once

#include "storage.h"

#include "core/mutex.h"
#include "core/spinlock.h"

#include <utility>

namespace storage {

class StorageLock {
 public:
  inline StorageLock(IStorage::LockType& lock_type) noexcept
      : lock_type_(lock_type) {}

  inline void acquire() noexcept {
    switch (lock_type_) {
      case IStorage::LockType::kMutex:
        mutex_.acquire();
        break;
      case IStorage::LockType::kSpinLock:
        mutex_.acquire();
        break;
      default:
        break;
    }
  }

  inline void release() noexcept {
    switch (lock_type_) {
      case IStorage::LockType::kMutex:
        mutex_.release();
        break;
      case IStorage::LockType::kSpinLock:
        lock_.release();
        break;
      default:
        break;
    }
  }

 private:
  storage::IStorage::LockType& lock_type_;

  core::Mutex mutex_;
  core::AdaptiveLock lock_;
};

}  // namespace storage