#include "core/assert.h"

#include "gtest/gtest.h"

TEST(AssertTest, AssertActsLikeFunctionTest) {
  if (true)
    core_assert(true);
  else
    core_assert(false);

  bool var = false;
  if (false)
    core_assert(false);
  else
    var = true;
  ASSERT_TRUE(var);
}

TEST(AssertTest, FailCompilesTest) {
  if (false) {
    core_fail("%d is magic number", 7);
    core_fail();
  }
}

TEST(AssertTest, VerifyCompilesTest) {
  core_verify(true, "hello %s", "there");
  core_verify(true);
}