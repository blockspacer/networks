#include "core/backtrace.h"

#include "gtest/gtest.h"

#include <sstream>

using PFunc = int (*)(void**, size_t);

int Dbg1(void** buf, size_t len) {
  volatile int ret = (int)core::BackTrace(buf, len);
  return ret;
}

int Dbg2(void** buf, size_t len) {
  volatile int ret = (int)core::BackTrace(buf, len);
  return ret;
}

void SomeMethod() {
  std::ostringstream out;
  core::FormatBackTrace(out);
  EXPECT_TRUE(out.str().empty() || out.str().find("SomeMethod") != std::string::npos) << out.str();
}

TEST(BackTraceTest, TestPrintBackTrace) { SomeMethod(); }

TEST(BackTraceTest, TestBackTrace) {
  void* buf1[100];
  size_t ret1;

  void* buf2[100];
  size_t ret2;

  volatile PFunc func = &Dbg1;
  ret1 = (*func)(buf1, 100);
  func = &Dbg2;
  ret2 = (*func)(buf2, 100);

  ASSERT_EQ(ret1, ret2);
}