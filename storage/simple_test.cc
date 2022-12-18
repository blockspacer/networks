#include "core/backtrace.h"
#include "core/exception.h"
#include "storage/api.h"

using proto::Message;

int main(int, char** argv) {
  try {
    auto storage = storage::CreateStorage({argv[1], argv[2]});

    Message a;  // = {"from1", {"to1", "to2"}, "hello", 10};
    Message b;  //= {"from2", {"to1", "to2", "to3"}, "hello", 20};

    a.set_from("from1");
    a.add_to("to1");
    a.set_send_ts(10);
    a.set_message("hello");

    b.set_from("from2");
    b.add_to("to2");
    b.add_to("to3");
    b.set_send_ts(2000);
    b.set_message("hello");
    b.add_reply("this is reply");

    storage->Store(a);
    storage->Store(b);

    auto r = storage->Load({"to1", "to3"});

    for (const auto& m : r) {
      std::cout << "\n-----------------------------------\n";
      std::cout << "From: " << m.from() << "\nTo:";
      for (const auto& t : m.to()) {
        std::cout << " " << t;
      }
      std::cout << "\nSend Time: " << m.send_ts() << "\nMessage: " << m.message()
                << "\nReply: " << (m.reply_size() == 0 ? "null" : m.reply(0)) << "\n";
    }

    std::cout << "\n\n******************************************************\n\n";

    auto rr = storage->LoadSended("from2");
    for (const auto& m : rr) {
      std::cout << "\n-----------------------------------\n";
      std::cout << "From: " << m.from() << "\nTo:";
      for (const auto& t : m.to()) {
        std::cout << " " << t;
      }
      std::cout << "\nSend Time: " << m.send_ts() << "\nMessage: " << m.message()
                << "\nReply: " << (m.reply_size() == 0 ? "null" : m.reply(0)) << "\n";
    }
  } catch (const core::Exception& e) {
    std::cout << e.what() << "\n";
    core::PrintBackTrace();
  }

  return 0;
}