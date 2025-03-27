#pragma once

#include "xllm_http_service.pb.h"

namespace xllm_service {

class XllmHttpServiceImpl : public proto::XllmHttpService {
 public:
  explicit XllmHttpServiceImpl();
  ~XllmHttpServiceImpl();

  void Hello(::google::protobuf::RpcController* controller,
             const proto::HttpRequest* request,
             proto::HttpResponse* response,
             ::google::protobuf::Closure* done) override;

  private:
};

} // xllm_service
