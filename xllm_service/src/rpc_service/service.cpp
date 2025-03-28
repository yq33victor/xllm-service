#include "rpc_service/service.h"
#include "types.h"

namespace xllm_service {

XllmService::XllmService() {
}

XllmService::~XllmService() {
}

ErrorCode XllmService::register_instance(const std::string& instance_name,
                                         const InstanceMetaInfo& metainfo) {
  return instance_mgr_.register_instance(instance_name, metainfo);
}

XllmServiceImpl::XllmServiceImpl(std::shared_ptr<XllmService> service)
    : xllm_service_(service) {
}

XllmServiceImpl::~XllmServiceImpl() {
}

grpc::Status XllmServiceImpl::Hello(
    grpc::ServerContext* context,
    const proto::Empty* req,
    proto::Status* resp) {
  resp->set_ok(true);
  return grpc::Status::OK;
}

grpc::Status XllmServiceImpl::RegisterInstance(
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

} // xllm_service
