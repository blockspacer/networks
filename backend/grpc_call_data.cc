#include "grpc_call_data.h"
#include "logging.h"
#include "user_groups.h"

#include "core/exception.h"

static auto ExpandUserName(std::string_view user_name) noexcept {
  auto r = backend::GetUserGroups(user_name);
  if (backend::GetUserType(user_name) == backend::LoginType::kUserName) {
    r.emplace_back(user_name);
  }
  r.emplace_back("#all");
  return r;
}

namespace backend {

ICallData::ICallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq, storage::IStorage* storage)
    : service_(service)
    , completion_queue_(cq)
    , storage_(storage)
    , status_(CallStatus::kCreate) {}

void ICallData::Proceed() {
  switch (status_) {
    case CallStatus::kCreate:
      DoCreate();
      break;
    case CallStatus::kProcess:
      DoProcess();
      break;
    case CallStatus::kFinish:
      DoFinish();
      break;
    default:
      GPR_ASSERT(status_ == CallStatus::kFinish);
      break;
  }
}

SendCallData::SendCallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq,
                           storage::IStorage* storage)
    : ICallData(service, cq, storage)
    , responder_(&context_) {
  Proceed();
}

ReceiveCallData::ReceiveCallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq,
                                 storage::IStorage* storage)
    : ICallData(service, cq, storage)
    , responder_(&context_) {
  Proceed();
}

FromCallData::FromCallData(proto::ChatRpc::AsyncService* service, grpc::ServerCompletionQueue* cq,
                           storage::IStorage* storage)
    : ICallData(service, cq, storage)
    , responder_(&context_) {
  Proceed();
}

void SendCallData::DoProcess() {
  new SendCallData(service_, completion_queue_, storage_);
  try {
    chat_server_log("start storing message");
    storage_->Store(request_.message());
    response_.set_status(proto::Status::kOk);
    chat_server_log("stop storing message");
  } catch (const core::Exception& e) {
    chat_server_log(e.what());
    class core::BackTrace bt;
    bt.capture();
    chat_server_log(bt.toString());
    response_.set_status(proto::Status::kError);
  }
  SetStatus(CallStatus::kFinish);
  responder_.Finish(response_, grpc::Status::OK, this);
}

void ReceiveCallData::DoProcess() {
  new ReceiveCallData(service_, completion_queue_, storage_);
  try {
    chat_server_log("start loading message for user");
    auto result = storage_->Load(ExpandUserName(request_.user()));
    *response_.mutable_messages() = {result.begin(), result.end()};
    response_.set_status(proto::Status::kOk);
    chat_server_log("finish loading message for user");
  } catch (const core::Exception& e) {
    chat_server_log(e.what());
    class core::BackTrace bt;
    bt.capture();
    chat_server_log(bt.toString());
    response_.set_status(proto::Status::kError);
  }
  SetStatus(CallStatus::kFinish);
  responder_.Finish(response_, grpc::Status::OK, this);
}

void FromCallData::DoProcess() {
  new FromCallData(service_, completion_queue_, storage_);
  try {
    chat_server_log("start loading sended messages for user");
    auto result = storage_->LoadSended(request_.user());
    *response_.mutable_messages() = {result.begin(), result.end()};
    response_.set_status(proto::Status::kOk);
    chat_server_log("finish loading sended messages for user");
  } catch (const core::Exception& e) {
    chat_server_log(e.what());
    class core::BackTrace bt;
    bt.capture();
    chat_server_log(bt.toString());
    response_.set_status(proto::Status::kError);
  }
  SetStatus(CallStatus::kFinish);
  responder_.Finish(response_, grpc::Status::OK, this);
}

}  // namespace backend