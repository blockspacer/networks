#include "core/intrusive_list.h"

#include "gtest/gtest.h"

class Int : public core::IntrusiveListItem<Int> {
 public:
  inline Int(int value) noexcept
      : value_(value) {}

  Int(Int&& rhs) noexcept
      : value_(rhs.value_) {
    rhs.value_ = 0xDEAD;
  }

  Int& operator=(Int&& rhs) noexcept {
    value_ = rhs.value_;
    rhs.value_ = 0xBEEF;
    return *this;
  }

  inline operator int&() noexcept { return value_; }

  inline operator const int&() const noexcept { return value_; }

 private:
  int value_;
};

class MyList : public core::IntrusiveList<Int> {
 public:
  inline MyList(int count) {
    while (count > 0) {
      pushFront(new Int(count--));
    }
  }

  MyList(MyList&& rhs) noexcept = default;

  MyList& operator=(MyList&& rhs) noexcept = default;

  inline ~MyList() {
    while (!empty()) {
      delete popBack();
    }
  }
};

struct IntGreater : private std::greater<int> {
  inline bool operator()(const Int& l, const Int& r) const noexcept { return std::greater<int>::operator()(l, r); }
};

class Sum {
 public:
  inline Sum(size_t& sum)
      : sum_(sum) {}

  inline void operator()(const Int* v) noexcept { sum_ += *v; }

 private:
  size_t& sum_;
};

class SumWithDelete {
 public:
  inline SumWithDelete(size_t& sum)
      : sum_(sum) {}

  inline void operator()(Int* v) noexcept {
    if (*v % 2) {
      sum_ += *v;
    } else {
      delete v;
    }
  }

 private:
  size_t& sum_;
};

static void CheckIterationAfterCut(const MyList& l, const MyList& l2, size_t n, size_t m) {
  size_t c = 0;
  for (MyList::ConstIterator it = l.begin(); it != l.end(); ++it) {
    ++c;

    ASSERT_EQ(*it, (int)c);
  }

  ASSERT_EQ(c, m);

  for (MyList::ConstIterator it = l2.begin(); it != l2.end(); ++it) {
    ++c;

    ASSERT_EQ(*it, (int)c);
  }

  ASSERT_EQ(c, n);

  for (MyList::ConstIterator it = l2.end(); it != l2.begin(); --c) {
    --it;

    ASSERT_EQ(*it, (int)c);
  }

  ASSERT_EQ(c, m);

  for (MyList::ConstIterator it = l.end(); it != l.begin(); --c) {
    --it;

    ASSERT_EQ(*it, (int)c);
  }

  ASSERT_EQ(c, 0);
}

static void TestCutFront(int n, int m) {
  MyList l(n);
  MyList l2(0);

  MyList::Iterator it = l.begin();
  for (int i = 0; i < m; ++i) {
    ++it;
  }

  MyList::cut(l.begin(), it, l2.end());
  CheckIterationAfterCut(l2, l, n, m);
}

static void TestCutBack(int n, int m) {
  MyList l(n);
  MyList l2(0);

  MyList::Iterator it = l.begin();
  for (int i = 0; i < m; ++i) {
    ++it;
  }

  MyList::cut(it, l.end(), l2.end());
  CheckIterationAfterCut(l, l2, n, m);
}

static void CheckIterationAfterAppend(const MyList& l, size_t n, size_t m) {
  MyList::ConstIterator it = l.begin();

  for (size_t i = 1; i <= n; ++i, ++it) {
    ASSERT_EQ((int)i, *it);
  }

  for (size_t i = 1; i <= m; ++i, ++it) {
    ASSERT_EQ((int)i, *it);
  }

  ASSERT_EQ(it, l.end());
}

static void TestAppend(int n, int m) {
  MyList l(n);
  MyList l2(m);
  l.append(l2);

  ASSERT_TRUE(l2.empty());
  CheckIterationAfterAppend(l, n, m);
}

template <typename ListType>
static void CheckList(const ListType& lst) {
  int i = 1;
  for (typename ListType::ConstIterator it = lst.begin(); it != lst.end(); ++it, ++i) {
    ASSERT_EQ(*it, i);
  }
}

class SelfCountingInt : public core::IntrusiveListItem<SelfCountingInt> {
 public:
  SelfCountingInt(int& counter, int value) noexcept
      : counter_(counter)
      , value_(value) {
    ++counter_;
  }

  SelfCountingInt(SelfCountingInt&& rhs) noexcept
      : counter_(rhs.counter_)
      , value_(rhs.value_) {
    rhs.value_ = 0xDEAD;
  }

  SelfCountingInt& operator=(SelfCountingInt&& rhs) noexcept {
    value_ = rhs.value_;
    rhs.value_ = 0xBEEF;
    return *this;
  }

  ~SelfCountingInt() noexcept { --counter_; }

  inline operator int&() noexcept { return value_; }

  inline operator const int&() const noexcept { return value_; }

 private:
  int& counter_;
  int value_;
};

struct SelfCountingIntDelete {
  static void destroy(SelfCountingInt* i) noexcept { delete i; }
};

TEST(IntrusiveListTest, TestSize) {
  MyList l(1000);
  ASSERT_EQ(l.size(), 1000);
}

TEST(IntrusiveListTest, TestIterate) {
  MyList l(1000);
  size_t c = 0;

  for (MyList::Iterator it = l.begin(); it != l.end(); ++it) {
    ++c;

    ASSERT_EQ(*it, (int)c);
  }

  ASSERT_EQ(c, 1000);
}

TEST(IntrusiveListTest, TestRIterate) {
  MyList l(1000);
  size_t c = 1000;

  ASSERT_EQ(l.rbegin(), MyList::ReverseIterator(l.end()));
  ASSERT_EQ(l.rend(), MyList::ReverseIterator(l.begin()));

  for (MyList::ReverseIterator it = l.rbegin(); it != l.rend(); ++it) {
    ASSERT_EQ(*it, (int)c--);
  }

  ASSERT_EQ(c, 0);
}

TEST(IntrusiveListTest, TestForEach) {
  MyList l(1000);
  size_t sum = 0;
  Sum functor(sum);

  l.forEach(functor);

  ASSERT_EQ(sum, 1000 * 1001 / 2);
}

TEST(IntrusiveListTest, TestForEachWithDelete) {
  MyList l(1000);
  size_t sum = 0;
  SumWithDelete functor(sum);

  l.forEach(functor);

  ASSERT_EQ(sum, 500 * 500);
}

TEST(IntrusiveListTest, TestQuickSort) {
  MyList l(1000);
  size_t c = 0;

  l.quickSort(IntGreater());

  for (auto& it : l) {
    ASSERT_EQ(it, int(1000 - c++));
  }
}

TEST(IntrusiveListTest, TestCut) {
  TestCutFront(1000, 500);
  TestCutBack(1000, 500);
  TestCutFront(1, 0);
  TestCutBack(1, 0);
  TestCutFront(1, 1);
  TestCutBack(1, 1);
  TestCutFront(2, 0);
  TestCutBack(2, 0);
  TestCutFront(2, 1);
  TestCutBack(2, 1);
  TestCutFront(2, 2);
  TestCutBack(2, 2);
}

TEST(IntrusiveListTest, TestAppend) {
  ::TestAppend(500, 500);
  ::TestAppend(0, 0);
  ::TestAppend(1, 0);
  ::TestAppend(0, 1);
  ::TestAppend(1, 1);
}

TEST(IntrusiveListTest, TestMoveCtor) {
  const int N{42};
  MyList lst{N};
  ASSERT_TRUE(!lst.empty());
  ASSERT_EQ(lst.size(), N);

  CheckList(lst);
  MyList nextLst{std::move(lst)};
  ASSERT_TRUE(lst.empty());
  CheckList(nextLst);
}

TEST(IntrusiveListTest, TestMoveOpEq) {
  const int N{42};
  MyList lst{N};
  ASSERT_TRUE(!lst.empty());
  ASSERT_EQ(lst.size(), N);
  CheckList(lst);

  const int M{2};
  MyList nextLst(M);
  ASSERT_TRUE(!nextLst.empty());
  ASSERT_EQ(nextLst.size(), M);
  CheckList(nextLst);

  nextLst = std::move(lst);
  ASSERT_TRUE(!nextLst.empty());
  ASSERT_EQ(nextLst.size(), N);
  CheckList(nextLst);
}

TEST(IntrusiveListTest, TestListWithAutoDelete) {
  using ListType = core::IntrusiveListWithAutoDelete<SelfCountingInt, SelfCountingIntDelete>;
  int counter{0};
  {
    ListType lst;
    ASSERT_TRUE(lst.empty());
    lst.pushFront(new SelfCountingInt(counter, 2));
    ASSERT_EQ(lst.size(), 1);
    ASSERT_EQ(counter, 1);
    lst.pushFront(new SelfCountingInt(counter, 1));
    ASSERT_EQ(lst.size(), 2);
    ASSERT_EQ(counter, 2);
    CheckList(lst);
  }

  ASSERT_EQ(counter, 0);
}

TEST(IntrusiveListTest, TestListWithAutoDeleteMoveCtor) {
  using ListType = core::IntrusiveListWithAutoDelete<SelfCountingInt, SelfCountingIntDelete>;
  int counter{0};
  {
    ListType lst;
    lst.pushFront(new SelfCountingInt(counter, 2));
    lst.pushFront(new SelfCountingInt(counter, 1));
    ASSERT_EQ(lst.size(), 2);
    ASSERT_EQ(counter, 2);
    CheckList(lst);

    ListType nextList(std::move(lst));
    ASSERT_EQ(nextList.size(), 2);
    CheckList(nextList);
    ASSERT_EQ(counter, 2);
  }

  ASSERT_EQ(counter, 0);
}

TEST(IntrusiveListTest, TestListWithAutoDeleteMoveOpEq) {
  using ListType = core::IntrusiveListWithAutoDelete<SelfCountingInt, SelfCountingIntDelete>;
  int counter{0};
  {
    ListType lst;
    lst.pushFront(new SelfCountingInt(counter, 2));
    lst.pushFront(new SelfCountingInt(counter, 1));
    ASSERT_EQ(lst.size(), 2);
    ASSERT_EQ(counter, 2);
    CheckList(lst);

    ListType nextList;
    ASSERT_TRUE(nextList.empty());
    nextList = std::move(lst);
    ASSERT_EQ(nextList.size(), 2);
    CheckList(nextList);
    ASSERT_EQ(counter, 2);
  }

  ASSERT_EQ(counter, 0);
}

TEST(IntrusiveListTest, TestListWithAutoDeleteClear) {
  using ListType = core::IntrusiveListWithAutoDelete<SelfCountingInt, SelfCountingIntDelete>;
  int counter{0};
  {
    ListType lst;
    ASSERT_TRUE(lst.empty());
    lst.pushFront(new SelfCountingInt(counter, 2));
    ASSERT_EQ(lst.size(), 1);
    ASSERT_EQ(counter, 1);
    lst.pushFront(new SelfCountingInt(counter, 1));
    ASSERT_EQ(lst.size(), 2);
    ASSERT_EQ(counter, 2);
    CheckList(lst);

    lst.clear();
    ASSERT_TRUE(lst.empty());
    ASSERT_EQ(counter, 0);
    lst.pushFront(new SelfCountingInt(counter, 1));
    ASSERT_EQ(lst.size(), 1);
  }

  ASSERT_EQ(counter, 0);
}