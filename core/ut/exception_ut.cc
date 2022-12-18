#include "core/exception.h"

#include "absl/strings/match.h"
#include "gtest/gtest.h"

static inline void Throw1DontMove() { core_throw core::Exception() << "blabla"; }

static inline void Throw2DontMove() { core_throw core::Exception() << 1 << " qw " << 12.1; }

TEST(ExceptionTest, TestRethrowAppend) {
  try {
    try {
      core_throw core::Exception() << "it";
    } catch (core::Exception& e) {
      e << "happens";
      throw;
    }
  } catch (...) {
    ASSERT_TRUE(absl::StrContains(core::CurrentExceptionMessage(), "ithappens"));
  }
}

TEST(ExceptionTest, TestNoException) { ASSERT_EQ(core::CurrentExceptionMessage(), "(NO EXCEPTION)"); }

TEST(ExceptionTest, TestBackTrace) {
  try {
    core_throw core::WithBackTrace<core::Exception>() << "it";
  } catch (core::Exception& e) {
    ASSERT_NE(core::CurrentExceptionMessage().find('\n'), std::string::npos);
    return;
  }
  FAIL() << "never";
}

TEST(ExceptionTest, TestLineInfo) {
  try {
    Throw1DontMove();
    FAIL() << "never";
  } catch (...) {
    ASSERT_TRUE(absl::StrContains(core::CurrentExceptionMessage(), "core/ut/exception_ut.cc:6: blabla"));
  }
}

TEST(ExceptionTest, TestRaise) {
  try {
    Throw2DontMove();
  } catch (...) {
    ASSERT_TRUE(absl::StrContains(core::CurrentExceptionMessage(), "core/ut/exception_ut.cc:8: 1 qw 12.1"));
  }
}