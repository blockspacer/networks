#pragma once

#include "proto/rpc_service.grpc.pb.h"
#include "storage/storage.h"

#include "grpcpp/grpcpp.h"

namespace backend {

class ICallData {
 public:
  enum class CallStatus { kCreate, kProcess, kFinish };

 public:
  ICallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq, storage::IStorage* storage);

  virtual ~ICallData() = default;

  void Proceed();

 protected:
  virtual void DoCreate() = 0;
  virtual void DoProcess() = 0;
  virtual void DoFinish() = 0;

  inline void SetStatus(CallStatus status) noexcept { status_ = status; }

 protected:
  proto::ChatRpc::AsyncService* service_;
  grpc::ServerCompletionQueue* completion_queue_;
  storage::IStorage* storage_;
  CallStatus status_;
};

class SendCallData final : public ICallData {
 public:
  SendCallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq, storage::IStorage* storage);

 private:
  void DoCreate() override {
    SetStatus(CallStatus::kProcess);
    service_->RequestSendMessage(&context_, &request_, &responder_, completion_queue_, completion_queue_, this);
  }

  void DoProcess() override;

  void DoFinish() override { delete this; }

 private:
  grpc::ServerContext context_;
  grpc::ServerAsyncResponseWriter<proto::SendResponse> responder_;

  proto::SendRequest request_;
  proto::SendResponse response_;
};

class ReceiveCallData final : public ICallData {
 public:
  ReceiveCallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq, storage::IStorage* storage);

 private:
  void DoCreate() override {
    SetStatus(CallStatus::kProcess);
    service_->RequestReceiveMessage(&context_, &request_, &responder_, completion_queue_, completion_queue_, this);
  }

  void DoProcess() override;

  void DoFinish() override { delete this; }

 private:
  grpc::ServerContext context_;
  grpc::ServerAsyncResponseWriter<proto::ReceiveResponse> responder_;

  proto::ReceiveRequest request_;
  proto::ReceiveResponse response_;
};

class FromCallData final : public ICallData {
 public:
  FromCallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq, storage::IStorage* storage);

 private:
  void DoCreate() override {
    SetStatus(CallStatus::kProcess);
    service_->RequestSendedMessages(&context_, &request_, &responder_, completion_queue_, completion_queue_, this);
  }

  void DoProcess() override;

  void DoFinish() override { delete this; }

 private:
  grpc::ServerContext context_;
  grpc::ServerAsyncResponseWriter<proto::FromResponse> responder_;

  proto::FromRequest request_;
  proto::FromResponse response_;
};

}  // namespace backend