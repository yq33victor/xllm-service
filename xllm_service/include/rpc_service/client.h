#pragma once
#include <grpcpp/grpcpp.h>
#include <string>
#include <thread>

#include "types.h"
#include "xllm_service.grpc.pb.h"

namespace xllm_service {

class XllmClient {
 public:
  XllmClient(const std::string& instace_name,
             const std::string& master_addr);
  ~XllmClient();

  ErrorCode register_instance();
  ErrorCode register_instance(const InstanceMetaInfo& metainfo);
 private:
  void heartbeat();

 private:
  bool exited_ = false;
  bool register_inst_done_ = false;
  // instance rdma address or other info: ip port
  std::string instance_name_;
  std::string master_addr_;
  std::unique_ptr<proto::XllmService::Stub> master_stub_;
  std::unique_ptr<std::thread> heartbeat_thread_;
};

} // xllm_service
