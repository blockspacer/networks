#include "core/atomic.h"

#include "gtest/gtest.h"

#include <limits>

using namespace core;

template <class T1, class T2, class T3>
struct Chooser {
  using Type = T2;
};

template <class T1, class T2>
struct Chooser<T1, T1, T2> {
  using Type = T2;
};

template <class T1>
struct Chooser<T1, T1, T1> {};

using AltAtomic = volatile Chooser<AtomicType, long, long long>::Type;

template <class TAtomic>
class AtomicTest : public testing::Test {
 protected:
  inline void TestLockUnlock() {
    TAtomic v = 0;

    ASSERT_TRUE(atomics::TryLock(&v));
    ASSERT_FALSE(atomics::TryLock(&v));
    ASSERT_EQ(v, 1);
    atomics::Unlock(&v);
    ASSERT_EQ(v, 0);
  }

  inline void TestCAS() {
    TAtomic v = 0;

    ASSERT_TRUE(atomics::Cas(&v, 1, 0));
    ASSERT_FALSE(atomics::Cas(&v, 1, 0));
    ASSERT_EQ(v, 1);
    ASSERT_TRUE(atomics::Cas(&v, 0, 1));
    ASSERT_EQ(v, 0);
    ASSERT_TRUE(atomics::Cas(&v, std::numeric_limits<intptr_t>::max(), 0));
    ASSERT_EQ(v, std::numeric_limits<intptr_t>::max());
  }

  inline void TestGetAndCAS() {
    TAtomic v = 0;

    ASSERT_EQ(atomics::GetAndCas(&v, 1, 0), 0);
    ASSERT_EQ(atomics::GetAndCas(&v, 2, 0), 1);
    ASSERT_EQ(v, 1);
    ASSERT_EQ(atomics::GetAndCas(&v, 0, 1), 1);
    ASSERT_EQ(v, 0);
    ASSERT_EQ(atomics::GetAndCas(&v, std::numeric_limits<intptr_t>::max(), 0), 0);
    ASSERT_EQ(v, std::numeric_limits<intptr_t>::max());
  }

  inline void TestAtomicInc1() {
    TAtomic v = 0;

    ASSERT_TRUE(atomics::Add(v, 1));
    ASSERT_EQ(v, 1);
    ASSERT_TRUE(atomics::Add(v, 10));
    ASSERT_EQ(v, 11);
  }

  inline void TestAtomicInc2() {
    TAtomic v = 0;

    ASSERT_TRUE(atomics::Increment(v));
    ASSERT_EQ(v, 1);
    ASSERT_TRUE(atomics::Increment(v));
    ASSERT_EQ(v, 2);
  }

  inline void TestAtomicGetAndInc() {
    TAtomic v = 0;

    ASSERT_EQ(atomics::GetAndIncrement(v), 0);
    ASSERT_EQ(v, 1);
    ASSERT_EQ(atomics::GetAndIncrement(v), 1);
    ASSERT_EQ(v, 2);
  }

  inline void TestAtomicDec() {
    TAtomic v = 2;

    ASSERT_TRUE(atomics::Decrement(v));
    ASSERT_EQ(v, 1);
    ASSERT_FALSE(atomics::Decrement(v));
    ASSERT_EQ(v, 0);
  }

  inline void TestAtomicGetAndDec() {
    TAtomic v = 2;

    ASSERT_EQ(atomics::GetAndDecrement(v), 2);
    ASSERT_EQ(v, 1);
    ASSERT_EQ(atomics::GetAndDecrement(v), 1);
    ASSERT_EQ(v, 0);
  }

  inline void TestAtomicAdd() {
    TAtomic v = 0;

    ASSERT_EQ(atomics::Add(v, 1), 1);
    ASSERT_EQ(atomics::Add(v, 2), 3);
    ASSERT_EQ(atomics::Add(v, -4), -1);
    ASSERT_EQ(v, -1);
  }

  inline void TestAtomicGetAndAdd() {
    TAtomic v = 0;

    ASSERT_EQ(atomics::GetAndAdd(v, 1), 0);
    ASSERT_EQ(atomics::GetAndAdd(v, 2), 1);
    ASSERT_EQ(atomics::GetAndAdd(v, -4), 3);
    ASSERT_EQ(v, -1);
  }

  inline void TestAtomicSub() {
    TAtomic v = 4;

    ASSERT_EQ(atomics::Sub(v, 1), 3);
    ASSERT_EQ(atomics::Sub(v, 2), 1);
    ASSERT_EQ(atomics::Sub(v, 3), -2);
    ASSERT_EQ(v, -2);
  }

  inline void TestAtomicGetAndSub() {
    TAtomic v = 4;

    ASSERT_EQ(atomics::GetAndSub(v, 1), 4);
    ASSERT_EQ(atomics::GetAndSub(v, 2), 3);
    ASSERT_EQ(atomics::GetAndSub(v, 3), 1);
    ASSERT_EQ(v, -2);
  }

  inline void TestAtomicSwap() {
    TAtomic v = 0;

    ASSERT_EQ(atomics::Swap(&v, 3), 0);
    ASSERT_EQ(atomics::Swap(&v, 5), 3);
    ASSERT_EQ(atomics::Swap(&v, -7), 5);
    ASSERT_EQ(atomics::Swap(&v, std::numeric_limits<intptr_t>::max()), -7);
    ASSERT_EQ(v, std::numeric_limits<intptr_t>::max());
  }

  inline void TestAtomicOr() {
    TAtomic v = 0xf0;

    ASSERT_EQ(atomics::Or(v, 0x0f), 0xff);
    ASSERT_EQ(v, 0xff);
  }

  inline void TestAtomicAnd() {
    TAtomic v = 0xff;

    ASSERT_EQ(atomics::And(v, 0xf0), 0xf0);
    ASSERT_EQ(v, 0xf0);
  }

  inline void TestAtomicXor() {
    TAtomic v = 0x00;

    ASSERT_EQ(atomics::Xor(v, 0xff), 0xff);
    ASSERT_EQ(atomics::Xor(v, 0xff), 0x00);
  }

  inline void TestAtomicPtr() {
    int* p;
    atomics::Store(p, nullptr);

    ASSERT_EQ(atomics::Load(p), nullptr);

    int i;
    atomics::Store(p, &i);

    ASSERT_EQ(atomics::Load(p), &i);
    ASSERT_EQ(atomics::Swap(&p, nullptr), &i);
    ASSERT_TRUE(atomics::Cas(&p, &i, nullptr));
  }
};

using TestTypes = testing::Types<Atomic, AltAtomic>;

TYPED_TEST_SUITE(AtomicTest, TestTypes);

TYPED_TEST(AtomicTest, AtomicInc1) { this->TestAtomicInc1(); }

TYPED_TEST(AtomicTest, AtomicInc2) { this->TestAtomicInc2(); }

TYPED_TEST(AtomicTest, AtomicGetAndInc) { this->TestAtomicGetAndInc(); }

TYPED_TEST(AtomicTest, AtomicDec) { this->TestAtomicDec(); }

TYPED_TEST(AtomicTest, AtomicGetAndDec) { this->TestAtomicGetAndDec(); }

TYPED_TEST(AtomicTest, AtomicAdd) { this->TestAtomicAdd(); }

TYPED_TEST(AtomicTest, AtomicGetAndAdd) { this->TestAtomicGetAndAdd(); }

TYPED_TEST(AtomicTest, AtomicSub) { this->TestAtomicSub(); }

TYPED_TEST(AtomicTest, AtomicGetAndSub) { this->TestAtomicGetAndSub(); }

TYPED_TEST(AtomicTest, AtomicSwap) { this->TestAtomicSwap(); }

TYPED_TEST(AtomicTest, AtomicOr) { this->TestAtomicOr(); }

TYPED_TEST(AtomicTest, AtomicAnd) { this->TestAtomicAnd(); }

TYPED_TEST(AtomicTest, AtomicXor) { this->TestAtomicXor(); }

TYPED_TEST(AtomicTest, AtomicPtr) { this->TestAtomicPtr(); }

TYPED_TEST(AtomicTest, CAS) { this->TestCAS(); }

TYPED_TEST(AtomicTest, GetAndCAS) { this->TestGetAndCAS(); }

TYPED_TEST(AtomicTest, LockUnlock) { this->TestLockUnlock(); }