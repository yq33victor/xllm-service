#include <brpc/closure_guard.h>

#include "rpc_service/service.h"
#include "types.h"

namespace xllm_service {

XllmServiceImpl::XllmServiceImpl(const std::string& etcd_addr) {
  instance_mgr_ = std::make_unique<InstanceMgr>(etcd_addr);
}

XllmServiceImpl::~XllmServiceImpl() {
}

ErrorCode XllmServiceImpl::heartbeat(const std::string& instance_name) {
  return instance_mgr_->heartbeat(instance_name);
}

ErrorCode XllmServiceImpl::register_instance(const std::string& instance_name,
                                             const InstanceMetaInfo& metainfo) {
  return instance_mgr_->register_instance(instance_name, metainfo);
}

ErrorCode XllmServiceImpl::update_instance_metainfo(const std::string& instance_name,
                                                    const InstanceMetaInfo& metainfo) {
  return instance_mgr_->update_instance_metainfo(instance_name, metainfo);
}

XllmService::XllmService(std::shared_ptr<XllmServiceImpl> service)
    : xllm_service_(service) {
}

XllmService::~XllmService() {
}

void XllmService::Hello(
    google::protobuf::RpcController* cntl_base,
    const proto::Empty* req,
    proto::Status* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  resp->set_ok(true);
}

void XllmService::RegisterInstance(
    google::protobuf::RpcController* cntl_base,
    const proto::InstanceMetaInfo* req,
    proto::StatusCode* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  InstanceType type = InstanceType::DEFAULT;
  if (req->has_type() && req->type() == proto::InstanceType::PREFILL) {
    type = InstanceType::PREFILL;
  } else if(req->has_type() && req->type() == proto::InstanceType::DECODE) {
    type = InstanceType::DECODE;
  }
  InstanceMetaInfo metainfo(req->name(), type);
  ErrorCode code = xllm_service_->register_instance(req->name(), metainfo);
  resp->set_status_code(ConvertErrorCode::to_int(code));
}

void XllmService::Heartbeat(
    google::protobuf::RpcController* cntl_base,
    const proto::HeartbeatRequest* req,
    proto::Status* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto inst_name = req->name();
  xllm_service_->heartbeat(inst_name);
  resp->set_ok(true);
}

} // namespace xllm_service
