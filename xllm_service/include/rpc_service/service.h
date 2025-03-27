#pragma once

#include "xllm_service.grpc.pb.h"

namespace xllm_service {

class XllmService final {
 public:
  explicit XllmService();
  ~XllmService();

 private:
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

 private:
  std::shared_ptr<XllmService> xllm_service_;
};

} // xllm_service
