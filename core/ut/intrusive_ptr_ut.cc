#include "core/intrusive_ptr.h"
#include "core/noncopyable.h"

#include "gtest/gtest.h"

#include <vector>

using namespace core;

namespace intrusive_convertion {

struct A : public AtomicRefCount<A> {};
struct AA : public A {};
struct B : public AtomicRefCount<B> {};

void Func(IntrusivePtr<A>) {}

void Func(IntrusivePtr<B>) {}

void Func(IntrusiveConstPtr<A>) {}

void Func(IntrusiveConstPtr<B>) {}

}  // namespace intrusive_convertion

class TOp : public AtomicRefCount<TOp>, public NonCopyable {
 public:
  static int Cnt;

 public:
  TOp() { ++Cnt; }
  virtual ~TOp() { --Cnt; }
};

int TOp::Cnt = 0;

class TOp2 : public TOp {
 public:
  IntrusivePtr<TOp> Op;

 public:
  TOp2(const IntrusivePtr<TOp>& op)
      : Op(op) {
    ++Cnt;
  }
  ~TOp2() override { --Cnt; }
};

class TOp3 {
 public:
  IntrusivePtr<TOp2> Op2;
};

void Attach(TOp3* op3, IntrusivePtr<TOp>* op) {
  IntrusivePtr<TOp2> op2 = new TOp2(*op);
  op3->Op2 = op2.get();
  *op = op2.get();
}

TEST(IntrusivePtrTest, TestIntrusive) {
  {
    IntrusivePtr<TOp> p, p2;
    TOp3 op3;
    {
      std::vector<IntrusivePtr<TOp>> f1;
      {
        std::vector<IntrusivePtr<TOp>> f2;
        f2.push_back(new TOp);
        p = new TOp;
        f2.push_back(p);
        Attach(&op3, &f2[1]);
        f1 = f2;
        ASSERT_EQ(f1[0]->refCount(), 2);
        ASSERT_EQ(f1[1]->refCount(), 3);
        ASSERT_EQ(f1[1].get(), op3.Op2.get());
        ASSERT_EQ(op3.Op2->refCount(), 3);
        ASSERT_EQ(op3.Op2->Op->refCount(), 2);
        ASSERT_EQ(TOp::Cnt, 4);
      }
      p2 = p;
    }
    ASSERT_EQ(op3.Op2->refCount(), 1);
    ASSERT_EQ(op3.Op2->Op->refCount(), 3);
    ASSERT_EQ(TOp::Cnt, 3);
  }
  ASSERT_EQ(TOp::Cnt, 0);
}

TEST(IntrusivePtrTest, TestIntrusiveConvertion) {
  using namespace intrusive_convertion;

  IntrusivePtr<AA> aa = new AA;

  ASSERT_EQ(aa->refCount(), 1);
  IntrusivePtr<A> a = aa;
  ASSERT_EQ(aa->refCount(), 2);
  ASSERT_EQ(a->refCount(), 2);
  aa.reset();
  ASSERT_EQ(a->refCount(), 1);

  Func(aa);
}

TEST(IntrusivePtrTest, TestIntrusiveConstConvertion) {
  using namespace intrusive_convertion;

  IntrusiveConstPtr<AA> aa = new AA;

  ASSERT_EQ(aa->refCount(), 1);
  IntrusiveConstPtr<A> a = aa;
  ASSERT_EQ(aa->refCount(), 2);
  ASSERT_EQ(a->refCount(), 2);
  aa.reset();
  ASSERT_EQ(a->refCount(), 1);

  Func(aa);
}

TEST(IntrusivePtrTest, TestMakeIntrusive) {
  {
    ASSERT_EQ(0, TOp::Cnt);
    auto p = MakeIntrusive<TOp>();
    ASSERT_EQ(1, p->refCount());
    ASSERT_EQ(1, TOp::Cnt);
  }
  ASSERT_EQ(TOp::Cnt, 0);
}