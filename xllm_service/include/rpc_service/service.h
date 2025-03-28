#pragma once

#include "instance_mgr.h"
#include "xllm_service.grpc.pb.h"

namespace xllm_service {

class XllmService final {
 public:
  explicit XllmService();
  ~XllmService();

  ErrorCode register_instance(const std::string& instance_name,
                              const InstanceMetaInfo& metainfo);

 private:
  InstanceMgr instance_mgr_;
};

// parse proto data and call XllmService
class XllmServiceImpl final : public proto::XllmService::Service {
 public:
  explicit XllmServiceImpl(std::shared_ptr<XllmService> service);
  ~XllmServiceImpl();

  grpc::Status Hello(
        grpc::ServerContext* context,
        const proto::Empty* req,
        proto::Status* resp) override;

  grpc::Status RegisterInstance(
        grpc::ServerContext* context,
        const proto::InstanceMetaInfo* req,
        proto::StatusCode* resp) override;

 private:
  std::shared_ptr<XllmService> xllm_service_;
};

} // xllm_service
