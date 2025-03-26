#include "service.h"

namespace xllm_service {

XllmService::XllmService() {
}

XllmService::~XllmService() {
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

} // xllm_service