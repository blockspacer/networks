#pragma once

#include "logging.h"

#include "core/atomic.h"
#include "core/event.h"
#include "proto/rpc_service.grpc.pb.h"
#include "storage/storage.h"

#include "grpcpp/grpcpp.h"

#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace backend {

class RpcServer {
 public:
  RpcServer(size_t threads_num, std::unique_ptr<storage::IStorage> storage)
      : storage_(std::move(storage)) {
    chat_server_log("starting rpc service");
    completion_queues_.reserve(threads_num);
    threads_.reserve(threads_num);
  }

  ~RpcServer() {
    chat_server_log("stopping rpc service");
    if (core::atomics::Load(is_running_)) {
      Stop();
    }
  }

  void Start(const std::string& server_address);

  void Stop();

  void WaitForStop();

 private:
  void ThreadWorker(grpc::ServerCompletionQueue* completion_queue);

 private:
  core::Atomic is_running_ = 0;

  core::ManualEvent stop_event_;
  proto::ChatRpc::AsyncService service_;

  std::unique_ptr<grpc::Server> server_;
  std::unique_ptr<storage::IStorage> storage_;

  std::vector<std::unique_ptr<grpc::ServerCompletionQueue>> completion_queues_;
  std::vector<std::thread> threads_;
};

}  // namespace backend