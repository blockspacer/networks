#include "core/guard.h"

#include "gtest/gtest.h"

#include <cstddef>

struct GuardChecker {
  void acquire() { guarded = true; }

  void release() { guarded = false; }

  bool guarded = false;
};

int WithLockIncrement(GuardChecker& m, int n) {
  core_with_lock(m) {
    EXPECT_TRUE(m.guarded);
    return n + 1;
  }
}

TEST(GuardTest, TestUnguard) {
  GuardChecker m;
  {
    auto g = core::Guard(m);
    ASSERT_TRUE(m.guarded);
    {
      auto unguard = core::Unguard(g);
      ASSERT_TRUE(!m.guarded);
    }
    ASSERT_TRUE(m.guarded);
  }

  {
    auto g = core::Guard(m);
    ASSERT_TRUE(m.guarded);
    {
      auto unguard = core::Unguard(g);
      ASSERT_TRUE(!m.guarded);
    }
    ASSERT_TRUE(m.guarded);
  }
}

TEST(GuardTest, TestMove) {
  GuardChecker m;
  size_t n = 0;
  {
    auto guard = core::Guard(m);

    ASSERT_TRUE(m.guarded);
    ++n;
  }

  ASSERT_TRUE(!m.guarded);
  ASSERT_TRUE(n == 1);
}

TEST(GuardTest, TestSync) {
  GuardChecker m;
  size_t n = 0;

  core_with_lock(m) {
    ASSERT_TRUE(m.guarded);
    ++n;
  }

  ASSERT_TRUE(!m.guarded);
  ASSERT_TRUE(n == 1);
}

TEST(GuardTest, TestGuard) {
  GuardChecker checker;

  ASSERT_TRUE(!checker.guarded);
  {
    core::Guarded<GuardChecker> guard(checker);
    ASSERT_TRUE(checker.guarded);
  }
  ASSERT_TRUE(!checker.guarded);
}

TEST(GuardTest, TestWithLock) {
  GuardChecker m;
  int n = 42;
  n = WithLockIncrement(m, n);
  ASSERT_TRUE(!m.guarded);
  ASSERT_TRUE(n == 43);
}

TEST(GuardTest, TestWithLockScope) {
  auto g = [](auto) {
    FAIL() << "Non global Guard used";
    return 0;
  };
  GuardChecker m;
  core_with_lock(m) { core_ignore_result(g); }
}