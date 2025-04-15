#pragma once

#include "instance_mgr.h"
#include "xllm_rpc_service.pb.h"

namespace xllm_service {

class XllmRpcServiceImpl final {
 public:
  XllmRpcServiceImpl(const std::string& etcd_addr);
  ~XllmRpcServiceImpl();
  ErrorCode heartbeat(const std::string& instance_name);

  ErrorCode register_instance(const std::string& instance_name,
                              const InstanceMetaInfo& metainfo);
  ErrorCode update_instance_metainfo(const std::string& instance_name,
                                     const InstanceMetaInfo& metainfo);

 private:
  std::unique_ptr<InstanceMgr> instance_mgr_;
};

// parse proto data and call XllmRpcService
class XllmRpcService : public proto::XllmRpcService {
 public:
  explicit XllmRpcService(std::shared_ptr<XllmRpcServiceImpl> service);
  virtual ~XllmRpcService();

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
  std::shared_ptr<XllmRpcServiceImpl> xllm_service_;
};

} // namespace xllm_service
