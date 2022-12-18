#include "proto/rpc_service.grpc.pb.h"

#include "grpcpp/grpcpp.h"

#include <iostream>
#include <string>

class Client {
 public:
  explicit Client(std::shared_ptr<grpc::Channel> channel)
      : stub_(proto::ChatRpc::NewStub(channel)) {}

  proto::Status Send(const proto::Message& msg) {
    proto::SendRequest request;
    proto::SendResponse response;

    *request.mutable_message() = msg;

    grpc::ClientContext context;
    grpc::CompletionQueue cq;
    grpc::Status status;

    std::unique_ptr<grpc::ClientAsyncResponseReader<proto::SendResponse>> rpc(
        stub_->PrepareAsyncSendMessage(&context, request, &cq));

    rpc->StartCall();
    rpc->Finish(&response, &status, (void*)1);

    void* tag;
    bool ok = false;

    GPR_ASSERT(cq.Next(&tag, &ok));
    GPR_ASSERT(tag == (void*)1);
    GPR_ASSERT(ok);

    if (status.ok()) {
      return response.status();
    } else {
      std::cerr << "Send RPC Failed\n";
      exit(1);
    }
  }

  proto::Status Receive(const std::string& user) {
    proto::ReceiveRequest request;
    proto::ReceiveResponse response;

    request.set_user(user);

    grpc::ClientContext context;
    grpc::CompletionQueue cq;
    grpc::Status status;

    std::unique_ptr<grpc::ClientAsyncResponseReader<proto::ReceiveResponse>> rpc(
        stub_->PrepareAsyncReceiveMessage(&context, request, &cq));

    rpc->StartCall();
    rpc->Finish(&response, &status, (void*)1);

    void* tag;
    bool ok = false;

    GPR_ASSERT(cq.Next(&tag, &ok));
    GPR_ASSERT(tag == (void*)1);
    GPR_ASSERT(ok);

    if (status.ok()) {
      for (const auto& m : response.messages()) {
        std::cout << "\n-----------------------------------\n";
        std::cout << "From: " << m.from() << "\nTo:";
        for (const auto& t : m.to()) {
          std::cout << " " << t;
        }
        std::cout << "\nSend Time: " << m.send_ts() << "\nMessage: " << m.message() << "\n";
      }
      return response.status();
    } else {
      std::cerr << "Send RPC Failed\n";
      exit(1);
    }
  }

 private:
  std::unique_ptr<proto::ChatRpc::Stub> stub_;
};

int main(int /*argc*/, char** argv) {
  proto::Message a;  // = {"from1", {"to1", "to2"}, "hello", 10};
  proto::Message b;  //= {"from2", {"to1", "to2", "to3"}, "hello", 20};

  a.set_from("from1");
  a.add_to("to1");
  a.set_send_ts(10);
  a.set_message("hello");

  b.set_from("from2");
  b.add_to("to2");
  b.add_to("to3");
  b.set_send_ts(2000);
  b.set_message("hello");

  Client client(grpc::CreateChannel(std::string("unix://") + argv[1], grpc::InsecureChannelCredentials()));

  if (client.Send(a) == proto::Status::kOk) {
    std::cout << "send ok\n";
  } else {
    std::cout << "send failed\n";
  }

  if (client.Send(b) == proto::Status::kOk) {
    std::cout << "send ok\n";
  } else {
    std::cout << "send failed\n";
  }

  client.Receive("to1");
}