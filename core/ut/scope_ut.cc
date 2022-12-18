#include "core/scope.h"

#include "gtest/gtest.h"

TEST(ScopeGuardTest, TestOnScopeExit) {
  int i = 0;

  {
    core_scope_exit(&i) { i *= 2; };

    core_scope_exit(&i) { i += 1; };
  }

  ASSERT_EQ(2, i);
}

TEST(ScopeGuardTest, TestDefer) {
  int i = 0;
  {
    core_defer { i = 20; };
  }
  ASSERT_EQ(i, 20);
}