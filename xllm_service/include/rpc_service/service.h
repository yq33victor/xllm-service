#pragma once

#include "instance_mgr.h"
#include "xllm_service.pb.h"

namespace xllm_service {

class XllmServiceImpl final {
 public:
  XllmServiceImpl(const std::string& etcd_addr);
  ~XllmServiceImpl();
  ErrorCode heartbeat(const std::string& instance_name);

  ErrorCode register_instance(const std::string& instance_name,
                              const InstanceMetaInfo& metainfo);
  ErrorCode update_instance_metainfo(const std::string& instance_name,
                                     const InstanceMetaInfo& metainfo);

 private:
  std::unique_ptr<InstanceMgr> instance_mgr_;
};

// parse proto data and call XllmService
class XllmService : public proto::XllmService {
 public:
  explicit XllmService(std::shared_ptr<XllmServiceImpl> service);
  virtual ~XllmService();

  virtual void Hello(
      google::protobuf::RpcController* cntl_base,
      const proto::Empty* req,
      proto::Status* resp,
      google::protobuf::Closure* done) override;

  virtual void RegisterInstance(
      google::protobuf::RpcController* cntl_base,
      const proto::InstanceMetaInfo* req,
      proto::StatusCode* resp,
      google::protobuf::Closure* done) override;

  virtual void Heartbeat(
      google::protobuf::RpcController* cntl_base,
      const proto::HeartbeatRequest* req,
      proto::Status* resp,
      google::protobuf::Closure* done) override;

 private:
  std::shared_ptr<XllmServiceImpl> xllm_service_;
};

} // namespace xllm_service
