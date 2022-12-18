#include "storage/in_memory/in_memory_storage.h"

#include "gtest/gtest.h"

using proto::Message;
using storage::InMemoryStorage;

TEST(InMemoryStorage, TestStoreLoad) {
  InMemoryStorage storage;

  Message a;  // = {"from1", {"to1", "to2"}, "hello", 10};
  Message b;  //= {"from2", {"to1", "to2", "to3"}, "hello", 20};

  a.set_from("from1");
  a.add_to("to1");
  a.set_send_ts(10);
  a.set_message("hello");

  b.set_from("from2");
  b.add_to("to2");
  b.add_to("to3");
  b.set_send_ts(20);
  b.set_message("hello");

  ASSERT_NO_THROW(storage.Store(a));
  ASSERT_NO_THROW(storage.Store(b));

  auto res1 = storage.Load({"to1"});
  auto res2 = storage.Load({"to2"});
  auto res3 = storage.Load({"to1", "to3"});

  ASSERT_EQ(res1.size(), 1);
  ASSERT_EQ(res2.size(), 1);
  ASSERT_EQ(res3.size(), 2);

  ASSERT_EQ(res1[0].from(), "from1");
  ASSERT_EQ(res2[0].from(), "from2");
}

TEST(InMemoryStorage, TestTimeCut) {
  InMemoryStorage storage;

  Message a;  // = {"from1", {"to1", "to2"}, "hello", 10};
  Message b;  //= {"from2", {"to1", "to2", "to3"}, "hello", 20};

  a.set_from("from1");
  a.add_to("to1");
  a.set_send_ts(10);
  a.set_message("hello");

  b.set_from("from2");
  b.add_to("to2");
  b.add_to("to3");
  b.set_send_ts(absl::ToUnixSeconds(absl::Now()) + 2000);
  b.set_message("hello");

  ASSERT_NO_THROW(storage.Store(a));
  ASSERT_NO_THROW(storage.Store(b));

  auto res1 = storage.Load({"to1"});
  auto res2 = storage.Load({"to2"});

  ASSERT_EQ(res1.size(), 1);
  ASSERT_EQ(res2.size(), 0);
}

TEST(InMemoryStorage, TestStoreLoadSended) {
  InMemoryStorage storage;

  Message a;  // = {"from1", {"to1", "to2"}, "hello", 10};
  Message b;  //= {"from2", {"to1", "to2", "to3"}, "hello", 20};

  a.set_from("from1");
  a.add_to("to1");
  a.set_send_ts(10);
  a.set_message("hello");

  b.set_from("from2");
  b.add_to("to2");
  b.add_to("to3");
  b.set_send_ts(20);
  b.set_message("hello");

  ASSERT_NO_THROW(storage.Store(a));
  ASSERT_NO_THROW(storage.Store(b));

  auto res1 = storage.LoadSended("from1");
  auto res2 = storage.LoadSended("from2");
  auto res3 = storage.LoadSended("from3");

  ASSERT_EQ(res1.size(), 1);
  ASSERT_EQ(res2.size(), 2);
  ASSERT_EQ(res3.size(), 0);

  ASSERT_EQ(res1[0].from(), "from1");
  ASSERT_EQ(res2[0].from(), "from2");
  ASSERT_EQ(res2[1].from(), "from2");
}