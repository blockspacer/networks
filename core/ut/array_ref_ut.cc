#include "core/array_ref.h"

#include "gtest/gtest.h"

#include <cstring>
#include <vector>

using namespace core;

template <class T>
static void Foo(const ConstArrayRef<T>) {}

static void Do(const ArrayRef<int> a) { a[0] = 8; }

TEST(ArrayRefTest, TestDefaultConstructor) {
  ArrayRef<int> defaulted;
  ASSERT_EQ(defaulted.data(), nullptr);
  ASSERT_EQ(defaulted.size(), 0u);
}

TEST(ArrayRefTest, TestConstructorFromArray) {
  int x[] = {10, 20, 30};
  ArrayRef<int> ref(x);
  ASSERT_EQ(3u, ref.size());
  ASSERT_EQ(30, ref[2]);
  ref[2] = 50;
  ASSERT_EQ(50, x[2]);

  ArrayRef<const int> const_ref(x);
  ASSERT_EQ(3u, const_ref.size());
  ASSERT_EQ(50, const_ref[2]);
  ref[0] = 100;
  ASSERT_EQ(const_ref[0], 100);
}

TEST(ArrayRefTest, TestAccessingElements) {
  int a[]{1, 2, 3};
  ArrayRef<int> ref(a);

  ASSERT_EQ(ref[0], 1);
  ASSERT_EQ(ref.at(0), 1);

  ref[0] = 5;
  ASSERT_EQ(a[0], 5);
}

TEST(ArrayRefTest, TestFrontBack) {
  const int x[] = {1, 2, 3};
  const ArrayRef<const int> rx{x};
  ASSERT_EQ(rx.front(), 1);
  ASSERT_EQ(rx.back(), 3);

  int y[] = {1, 2, 3};
  ArrayRef<int> ry{y};
  ASSERT_EQ(ry.front(), 1);
  ASSERT_EQ(ry.back(), 3);

  ry.front() = 100;
  ry.back() = 500;
  ASSERT_EQ(ry.front(), 100);
  ASSERT_EQ(ry.back(), 500);
  ASSERT_EQ(y[0], 100);
  ASSERT_EQ(y[2], 500);
}

TEST(ArrayRefTest, TestIterator) {
  int array[] = {17, 19, 21};
  ArrayRef<int> r(array, 3);

  ArrayRef<int>::iterator iterator = r.begin();
  for (auto& i : array) {
    ASSERT_TRUE(iterator != r.end());
    ASSERT_EQ(i, *iterator);
    ++iterator;
  }
  ASSERT_TRUE(iterator == r.end());
}

TEST(ArrayRefTest, TestReverseIterators) {
  const int x[] = {1, 2, 3};
  const ArrayRef<const int> rx{x};
  auto i = rx.crbegin();
  ASSERT_EQ(*i, 3);
  ++i;
  ASSERT_EQ(*i, 2);
  ++i;
  ASSERT_EQ(*i, 1);
  ++i;
  ASSERT_EQ(i, rx.crend());
}

TEST(ArrayRefTest, TestConstIterators) {
  int x[] = {1, 2, 3};
  ArrayRef<int> rx{x};
  ASSERT_EQ(rx.begin(), rx.cbegin());
  ASSERT_EQ(rx.end(), rx.cend());
  ASSERT_EQ(rx.rbegin(), rx.crbegin());
  ASSERT_EQ(rx.rend(), rx.crend());

  int w[] = {1, 2, 3};
  const ArrayRef<int> rw{w};
  ASSERT_EQ(rw.begin(), rw.cbegin());
  ASSERT_EQ(rw.end(), rw.cend());
  ASSERT_EQ(rw.rbegin(), rw.crbegin());
  ASSERT_EQ(rw.rend(), rw.crend());

  int y[] = {1, 2, 3};
  ArrayRef<const int> ry{y};
  ASSERT_EQ(ry.begin(), ry.cbegin());
  ASSERT_EQ(ry.end(), ry.cend());
  ASSERT_EQ(ry.rbegin(), ry.crbegin());
  ASSERT_EQ(ry.rend(), ry.crend());

  const int z[] = {1, 2, 3};
  ArrayRef<const int> rz{z};
  ASSERT_EQ(rz.begin(), rz.cbegin());
  ASSERT_EQ(rz.end(), rz.cend());
  ASSERT_EQ(rz.rbegin(), rz.crbegin());
  ASSERT_EQ(rz.rend(), rz.crend());

  const int q[] = {1, 2, 3};
  const ArrayRef<const int> rq{q};
  ASSERT_EQ(rq.begin(), rq.cbegin());
  ASSERT_EQ(rq.end(), rq.cend());
  ASSERT_EQ(rq.rbegin(), rq.crbegin());
  ASSERT_EQ(rq.rend(), rq.crend());
}

TEST(ArrayRefTest, TestCreatingFromStringLiteral) {
  ConstArrayRef<char> known_size_ref("123", 3);
  size_t ret = 0;

  for (char ch : known_size_ref) {
    ret += ch - '0';
  }

  ASSERT_EQ(ret, 6);
  ASSERT_EQ(known_size_ref.size(), 3);
  ASSERT_EQ(known_size_ref.at(0), '1');

  ConstArrayRef<char> auto_size_ref("456");
  ASSERT_EQ(auto_size_ref[0], '4');
  ASSERT_EQ(auto_size_ref[3], '\0');
}

TEST(ArrayRefTest, TestEqualityOperator) {
  static constexpr size_t size = 5;
  int a[size]{1, 2, 3, 4, 5};
  int b[size]{5, 4, 3, 2, 1};
  int c[size - 1]{5, 4, 3, 2};
  float d[size]{1.f, 2.f, 3.f, 4.f, 5.f};

  ArrayRef<int> a_ref(a);
  ConstArrayRef<int> a_const_ref(a, size);

  ArrayRef<int> b_ref(b);

  ArrayRef<int> c_ref(c, size - 1);

  ArrayRef<float> d_ref(d, size);
  ConstArrayRef<float> d_const_ref(d, size);

  ASSERT_EQ(a_ref, a_const_ref);
  ASSERT_EQ(d_ref, d_const_ref);

  ASSERT_FALSE(a_ref == c_ref);
  ASSERT_FALSE(a_ref == b_ref);

  ArrayRef<int> b_sub_ref(b, size - 1);

  ASSERT_EQ(c_ref, b_sub_ref);
}

TEST(ArrayRefTest, TestImplicitConstructionFromContainer) {
  auto fc = [](ArrayRef<const int>) {};
  auto fm = [](ArrayRef<int>) {};

  fc(std::vector<int>({1}));

  const std::vector<int> ac = {1};
  std::vector<int> am = {1};

  fc(ac);
  fc(am);
  fm(am);
}

TEST(ArrayRefTest, TestFirstLastSubspan) {
  const int arr[] = {1, 2, 3, 4, 5};
  ArrayRef<const int> aRef(arr);

  ASSERT_EQ(aRef.first(2), MakeArrayRef(std::vector<int>{1, 2}));
  ASSERT_EQ(aRef.last(2), MakeArrayRef(std::vector<int>{4, 5}));
  ASSERT_EQ(aRef.subSpan(2), MakeArrayRef(std::vector<int>{3, 4, 5}));
  ASSERT_EQ(aRef.subSpan(1, 3), MakeArrayRef(std::vector<int>{2, 3, 4}));
}

TEST(ArrayRefTest, TestSlice) {
  const int a0[] = {1, 2, 3};
  ArrayRef<const int> r0(a0);
  ArrayRef<const int> s0 = r0.subSpan(2);

  ASSERT_EQ(s0.size(), 1);
  ASSERT_EQ(s0[0], 3);

  const int a1[] = {1, 2, 3, 4};
  ArrayRef<const int> r1(a1);
  ArrayRef<const int> s1 = r1.subSpan(2, 1);

  ASSERT_EQ(s1.size(), 1);
  ASSERT_EQ(s1[0], 3);
}

TEST(ArrayRefTest, SubRegion) {
  std::vector<char> x;
  for (size_t i = 0; i < 42; ++i) {
    x.push_back('a' + (i * 42424243) % 13);
  }
  ArrayRef<const char> ref(x.data(), 42);
  for (size_t i = 0; i <= 50; ++i) {
    std::vector<char> expected;
    for (size_t j = 0; j <= 100; ++j) {
      ASSERT_TRUE(MakeArrayRef(expected) == ref.subRegion(i, j));
      if (i + j < 42) {
        expected.push_back(x[i + j]);
      }
    }
  }
}

TEST(ArrayRefTest, TestAsBytes) {
  const int16_t const_arr[] = {1, 2, 3};
  ArrayRef<const int16_t> const_ref(const_arr);
  auto bytes_ref = AsBytes(const_ref);

  ASSERT_EQ(bytes_ref.size(), sizeof(int16_t) * const_ref.size());
  ASSERT_EQ(bytes_ref, MakeArrayRef(std::vector<char>{0x01, 0x00, 0x02, 0x00, 0x03, 0x00}));
}

TEST(ArrayRefTest, TestAsWritableBytes) {
  uint32_t uint_arr[] = {0x0c'00'0d'0e};
  ArrayRef<uint32_t> uint_ref(uint_arr);
  auto writable_bytes_ref = AsWritableBytes(uint_ref);

  ASSERT_EQ(writable_bytes_ref.size(), sizeof(uint32_t));
  ASSERT_EQ(writable_bytes_ref, MakeArrayRef(std::vector<char>{0x0e, 0x0d, 0x00, 0x0c}));

  uint32_t newVal = 0xde'ad'be'ef;
  std::memcpy(writable_bytes_ref.data(), &newVal, writable_bytes_ref.size());
  ASSERT_EQ(uint_arr[0], newVal);
}

TEST(ArrayRefTest, TestTypeDeductionViaMakeArrayRef) {
  std::vector<int> vec{17, 19, 21};
  ArrayRef<int> ref = MakeArrayRef(vec);
  ASSERT_EQ(21, ref[2]);
  ref[1] = 23;
  ASSERT_EQ(23, vec[1]);

  const std::vector<int>& const_vec(vec);
  ArrayRef<const int> constRef = MakeArrayRef(const_vec);
  ASSERT_EQ(21, constRef[2]);

  ArrayRef<const int> constRefFromNonConst = MakeArrayRef(vec);
  ASSERT_EQ(23, constRefFromNonConst[1]);
}

TEST(ArrayRefTest, TestConst) {
  int a[] = {1, 2};
  Do(a);
  ASSERT_EQ(a[0], 8);
}

TEST(ArrayRefTest, TestConstexpr) {
  static constexpr const int a[] = {1, 2, -3, -4};
  static constexpr const auto r0 = MakeArrayRef(a, 1);
  static_assert(r0.size() == 1, "r0.size() is not equal 1");
  static_assert(r0.data()[0] == 1, "r0.data()[0] is not equal to 1");

  static constexpr const ArrayRef<const int> r1{a};
  static_assert(r1.size() == 4, "r1.size() is not equal to 4");
  static_assert(r1.data()[3] == -4, "r1.data()[3] is not equal to -4");

  static constexpr const ArrayRef<const int> r2 = r1;
  static_assert(r2.size() == 4, "r2.size() is not equal to 4");
  static_assert(r2.data()[2] == -3, "r2.data()[2] is not equal to -3");
}

TEST(ArrayRefTest, TestMakeConstArrayRef) {
  std::vector<int> data;

  Foo(MakeConstArrayRef(data));

  const std::vector<int> constData;
  Foo(MakeConstArrayRef(constData));
}
