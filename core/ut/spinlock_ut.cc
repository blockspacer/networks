#include "core/spinlock.h"

#include "gtest/gtest.h"

template <class Lock>
class LockTest : public testing::Test {
 protected:
  inline void TestLock() {
    Lock lock;
    ASSERT_FALSE(lock.isLocked());
    lock.acquire();
    ASSERT_TRUE(lock.isLocked());
    lock.release();
    ASSERT_FALSE(lock.isLocked());

    ASSERT_TRUE(lock.tryAcquire());
    ASSERT_TRUE(lock.isLocked());
    ASSERT_FALSE(lock.tryAcquire());
    ASSERT_TRUE(lock.isLocked());
    lock.release();
    ASSERT_FALSE(lock.isLocked());
  }
};

using TestTypes = testing::Types<core::SpinLock, core::AdaptiveLock>;

TYPED_TEST_SUITE(LockTest, TestTypes);

TYPED_TEST(LockTest, TestLock) { this->TestLock(); }