#include <brpc/controller.h>
#include <glog/logging.h>

#include "http_service/service.h"

namespace xllm_service {

XllmHttpServiceImpl::XllmHttpServiceImpl() {
}

XllmHttpServiceImpl::~XllmHttpServiceImpl() {
}

void XllmHttpServiceImpl::Hello(::google::protobuf::RpcController* controller,
                                const proto::HttpRequest* request,
                                proto::HttpResponse* response,
                                ::google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  if (!request || !response || !controller) {
    LOG(ERROR) << "brpc request | respose | controller is null";
    return;
  }

  LOG(INFO) << "Get request: " << request->ping();
  
  response->set_pong(request->ping());
}

} // xllm_service
