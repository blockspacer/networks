#include "core/thread.h"
#include "core/tls.h"

#include "gtest/gtest.h"

namespace {

struct X {
  inline X()
      : v(0) {}

  inline void doSomething() { ++core::TlsRef(v); }

  inline int get() { return core::TlsRef(v); }

  core_thread(int) v;
};

struct Thr : public core::ISimpleThread {
  inline Thr(X* x)
      : ptr(x) {}

  void* threadProc() noexcept override {
    for (size_t i = 0; i < 100000; ++i) {
      ptr[i].doSomething();
    }
    return nullptr;
  }

  X* ptr;
};

}  // namespace

TEST(TlsTest, TestHugeSetup) {
  std::unique_ptr<X[]> x(new X[100000]);
  Thr t1(x.get());
  Thr t2(x.get());

  t1.start();
  t2.start();

  t1.join();
  t2.join();

  for (size_t i = 0; i < 100000; ++i) {
    ASSERT_EQ(x.get()[i].get(), 0);
  }
}