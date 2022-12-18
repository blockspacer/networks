#include "server.h"
#include "grpc_call_data.h"

namespace backend {

void RpcServer::Start(const std::string& server_address) {
  grpc::ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials())
      .RegisterService(&service_)
      .SetResourceQuota(grpc::ResourceQuota("threads").SetMaxThreads(threads_.capacity()));

  for (size_t i = 0; i < threads_.capacity(); ++i) {
    completion_queues_.emplace_back(builder.AddCompletionQueue());
  }

  core::atomics::Store(is_running_, 1);
  server_ = builder.BuildAndStart();

  for (size_t i = 0; i < threads_.capacity(); ++i) {
    threads_.emplace_back(&RpcServer::ThreadWorker, this, completion_queues_[i].get());
  }
}

void RpcServer::Stop() {
  core::atomics::Store(is_running_, 0);
  server_->Shutdown();
  for (const auto& cq : completion_queues_) {
    cq->Shutdown();
  }
  for (auto& t : threads_) {
    t.join();
  }
  stop_event_.signal();
}

void RpcServer::WaitForStop() { stop_event_.wait(); }

void RpcServer::ThreadWorker(grpc::ServerCompletionQueue* completion_queue) {
  new SendCallData(&service_, completion_queue, storage_.get());
  new ReceiveCallData(&service_, completion_queue, storage_.get());
  new FromCallData(&service_, completion_queue, storage_.get());

  void* tag;
  bool ok;

  while (completion_queue->Next(&tag, &ok)) {
    if (ok) {
      static_cast<ICallData*>(tag)->Proceed();
    }
  }
}

}  // namespace backend