#include "rpc_service/service.h"
#include "types.h"

namespace xllm_service {

XllmServiceImpl::XllmServiceImpl() {
}

XllmServiceImpl::~XllmServiceImpl() {
}

ErrorCode XllmServiceImpl::heartbeat(const std::string& instance_name) {
  return instance_mgr_.heartbeat(instance_name);
}

ErrorCode XllmServiceImpl::register_instance(const std::string& instance_name,
                                             const InstanceMetaInfo& metainfo) {
  return instance_mgr_.register_instance(instance_name, metainfo);
}

XllmService::XllmService(std::shared_ptr<XllmServiceImpl> service)
    : xllm_service_(service) {
}

XllmService::~XllmService() {
}

grpc::Status XllmService::Hello(
    grpc::ServerContext* context,
    const proto::Empty* req,
    proto::Status* resp) {
  resp->set_ok(true);
  return grpc::Status::OK;
}

grpc::Status XllmService::RegisterInstance(
    grpc::ServerContext* context,
    const proto::InstanceMetaInfo* req,
    proto::StatusCode* resp) {
  InstanceType type = InstanceType::DEFAULT;
  if (req->has_type() && req->type() == proto::InstanceType::PREFILL) {
    type = InstanceType::PREFILL;
  } else if(req->has_type() && req->type() == proto::InstanceType::DECODE) {
    type = InstanceType::DECODE;
  }
  InstanceMetaInfo metainfo(req->name(), type);
  ErrorCode code = xllm_service_->register_instance(req->name(), metainfo);
  resp->set_status_code(ConvertErrorCode::to_int(code));
  return grpc::Status::OK;
}

grpc::Status XllmService::Heartbeat(
    grpc::ServerContext* context,
    const proto::HeartbeatRequest* req,
    proto::Status* resp) {
  auto inst_name = req->name();
  xllm_service_->heartbeat(inst_name);
  resp->set_ok(true);
  return grpc::Status::OK;
}

} // xllm_service
