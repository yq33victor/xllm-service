#pragma once

#include "instance_mgr.h"
#include "xllm_service.grpc.pb.h"

namespace xllm_service {

class XllmServiceImpl final {
 public:
  explicit XllmServiceImpl();
  ~XllmServiceImpl();
  ErrorCode heartbeat(const std::string& instance_name);

  ErrorCode register_instance(const std::string& instance_name,
                              const InstanceMetaInfo& metainfo);

 private:
  InstanceMgr instance_mgr_;
};

// parse proto data and call XllmService
class XllmService final : public proto::XllmService::Service {
 public:
  explicit XllmService(std::shared_ptr<XllmServiceImpl> service);
  ~XllmService();

  grpc::Status Hello(
        grpc::ServerContext* context,
        const proto::Empty* req,
        proto::Status* resp) override;

  grpc::Status RegisterInstance(
        grpc::ServerContext* context,
        const proto::InstanceMetaInfo* req,
        proto::StatusCode* resp) override;

  grpc::Status Heartbeat(
        grpc::ServerContext* context,
        const proto::HeartbeatRequest* req,
        proto::Status* resp) override;

 private:
  std::shared_ptr<XllmServiceImpl> xllm_service_;
};

} // xllm_service
