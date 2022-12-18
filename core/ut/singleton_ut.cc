#include "core/singleton.h"

#include "gtest/gtest.h"

#include <string>
#include <utility>

struct Huge {
  char buf[1000000];
  int v = 1234;
};

struct WithParams {
  explicit WithParams(const uint32_t data1 = 0, std::string data2 = std::string())
      : data_1(data1)
      , data_2(std::move(data2)) {}

  uint32_t data_1;
  std::string data_2;
};

using namespace core;

TEST(SingletonTest, TestHuge) {
  ASSERT_EQ(*HugeSingleton<int>(), 0);
  ASSERT_EQ(HugeSingleton<Huge>()->v, 1234);
}

TEST(SingletonTest, TestConstructorParamsOrder) {
  ASSERT_EQ(Singleton<WithParams>(10, "123123")->data_1, 10);
  ASSERT_EQ(Singleton<WithParams>(20, "123123")->data_1, 10);
  ASSERT_EQ(Singleton<WithParams>(10, "456456")->data_2, "123123");
}

TEST(SingletonTest, TestInstantiationWithConstructorParams) {
  ASSERT_EQ(Singleton<WithParams>(10)->data_1, 10);
  ASSERT_EQ(HugeSingleton<WithParams>(20, "123123")->data_2, "123123");
  {
    const auto value = SingletonWithPriority<WithParams, 12312>(30, "456")->data_1;
    ASSERT_EQ(value, 30);
  }
  {
    const auto value = HugeSingletonWithPriority<WithParams, 12311>(40, "789")->data_2;
    ASSERT_EQ(value, "789");
  }
  ASSERT_EQ(Default<WithParams>().data_1, 0);
}