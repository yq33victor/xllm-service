#include <glog/logging.h>

#include "rpc_service/client.h"

namespace xllm_service {

// magic number, TODO: move to config file or env var
static constexpr int kHeartbeatInterval = 3; // in seconds

XllmClient::XllmClient(const std::string& instace_name,
                       const std::string& master_addr)
    : instance_name_(instace_name), master_addr_(master_addr) {
  auto channel =
      grpc::CreateChannel(master_addr_, grpc::InsecureChannelCredentials());
  master_stub_ = proto::XllmService::NewStub(channel);
  CHECK(master_stub_) << "failed to create master stub for " << master_addr_;

  // heartbeat thread
  heartbeat_thread_ = std::make_unique<std::thread>(&XllmClient::heartbeat, this);
}

XllmClient::~XllmClient() {
  exited_ = true;
  if (heartbeat_thread_) {
    heartbeat_thread_->join();
  }
}

// TODO: send metainfo/metrics to master ? 
void XllmClient::heartbeat() {
  while (!exited_) {
    std::this_thread::sleep_for(std::chrono::seconds(kHeartbeatInterval));
    if (!register_inst_done_) continue;

    proto::HeartbeatRequest req;
    req.set_name(instance_name_);
    proto::Status res;
    grpc::ClientContext ctx;
    grpc::Status status = master_stub_->Heartbeat(&ctx, req, &res);
    if (!status.ok() || !res.ok()) {
      LOG(ERROR) << instance_name_
                 << " failed to send heartbeat to master, status: "
                 << status.error_message();
    }
  }
}

ErrorCode XllmClient::register_instance() {
  InstanceMetaInfo metainfo;
  metainfo.name = instance_name_;
  return register_instance(metainfo);
}

ErrorCode XllmClient::register_instance(const InstanceMetaInfo& metainfo) {
  proto::InstanceMetaInfo req;
  req.set_name(metainfo.name);
  if (metainfo.type == InstanceType::PREFILL) {
    req.set_type(proto::InstanceType::PREFILL);
  } else if (metainfo.type == InstanceType::DECODE) {
    req.set_type(proto::InstanceType::DECODE);
  } else {
    req.set_type(proto::InstanceType::DEFAULT);
  }
  proto::StatusCode res;
  grpc::ClientContext ctx;
  grpc::Status status = master_stub_->RegisterInstance(&ctx, req, &res);
  if (!status.ok() || res.status_code() != ConvertErrorCode::to_int(ErrorCode::OK)) {
    LOG(ERROR) << instance_name_
               << " failed to send register_instance to master, status: "
               << status.error_message() << ", res: " << res.status_code();
  } else {
    // register instance success
    register_inst_done_ = true;
  }
  return ConvertErrorCode::from_int(res.status_code());
}

} // xllm_service
