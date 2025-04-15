#include <brpc/controller.h>
#include <glog/logging.h>
#include <nlohmann/json.hpp>

#include "http_service/call_data.h"
#include "http_service/service.h"
#include "rpc_service/service.h"

namespace xllm_service {

namespace {
// TODO: use num_threads params later
static const size_t NUM_THREADS = 32;
}

XllmHttpServiceImpl::XllmHttpServiceImpl(std::shared_ptr<XllmRpcServiceImpl> rpc_service)
    : rpc_service_(rpc_service) {
  initialized_ = true;
  thread_pool_ = std::make_unique<ThreadPool>(NUM_THREADS);
}

XllmHttpServiceImpl::XllmHttpServiceImpl() {
  initialized_ = true;
  thread_pool_ = std::make_unique<ThreadPool>(NUM_THREADS);
}

XllmHttpServiceImpl::~XllmHttpServiceImpl() {
}

void XllmHttpServiceImpl::create_channel(const std::string& target_uri) {
  std::lock_guard<std::mutex> guard(channel_mutex_);
  if (cached_channels_.find(target_uri) == cached_channels_.end()) {
    brpc::Channel* channel = new brpc::Channel();
    brpc::ChannelOptions options;
    // Add to params
    options.protocol = "http";
    options.timeout_ms = 10000; /*milliseconds*/
    options.max_retry = 3;
    std::string load_balancer = "";
    if (channel->Init(target_uri.c_str(), load_balancer.c_str(), &options) != 0) {
      LOG(ERROR) << "Fail to initialize channel for " << target_uri;
      return;
    }
    cached_channels_[target_uri] = channel;
  }
}

void XllmHttpServiceImpl::Hello(::google::protobuf::RpcController* controller,
                                const proto::HttpHelloRequest* request,
                                proto::HttpHelloResponse* response,
                                ::google::protobuf::Closure* done) {
  assert(initialized_);
  brpc::ClosureGuard done_guard(done);
  if (!request || !response || !controller) {
    LOG(ERROR) << "brpc request | respose | controller is null";
    return;
  }

  LOG(INFO) << "Get request: " << request->ping();
  
  response->set_pong(request->ping());
}

void XllmHttpServiceImpl::Completions(::google::protobuf::RpcController* controller,
                                      const proto::HttpRequest* request,
                                      proto::HttpResponse* response,
                                      ::google::protobuf::Closure* done) {
  assert(initialized_);
  if (!request || !response || !controller) {
    LOG(ERROR) << "brpc request | respose | controller is null";
    return;
  }

  auto cntl = reinterpret_cast<brpc::Controller*>(controller);
  std::string req_attachment = cntl->request_attachment().to_string();
  nlohmann::json json_value = nlohmann::json::parse(req_attachment);
  bool stream = false;
  if (json_value.contains("stream")) {
    try {
      stream = json_value.at("stream").get<bool>();
    } catch  (const std::exception& e) {
      // pass
    }
    try {
      stream = json_value.at("stream").get<int>() == 1;
    } catch  (const std::exception& e) {
      LOG(ERROR) << "Error stream field in request, required bool or int value.";
      return;
    }
  }
  auto call_data = std::make_shared<StreamCallDataBrpc>(cntl, stream, done);

  // redistribute the request to the correct P/D instance
  // TODO: redistribute policy to select the instance
  std::string target_instance_addr = "127.0.0.1:9999";
  std::string target_uri = target_instance_addr + "/v1/completions";
  if (cached_channels_.find(target_uri) == cached_channels_.end()) {
    create_channel(target_uri);
  }

  // async redistribute the request and wait the response
  // TODO: optimize the thread pool to async mode.
  auto channel_ptr = cached_channels_[target_uri];
  thread_pool_->schedule([stream, req_attachment, call_data, cntl, channel_ptr, target_uri]() {
    brpc::Controller redirect_cntl;
    redirect_cntl.http_request().uri() = target_uri.c_str();
    redirect_cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    // redirect the input request content
    redirect_cntl.request_attachment().append(req_attachment);

    // Because `done'(last parameter) is NULL, this function waits until
    // the response comes back or error occurs(including timeout).
    channel_ptr->CallMethod(NULL, &redirect_cntl, NULL, NULL, NULL);
    if (redirect_cntl.Failed()) {
      LOG(ERROR) << "Redirect to instance error: " << redirect_cntl.ErrorText();
      call_data->finish_with_error(redirect_cntl.ErrorText());
      return;
    }

    if (stream) {
      //if (!call_data->write(redirect_cntl.response_attachment())) {
      if (!call_data->write(redirect_cntl.response_attachment().to_string())) {
        LOG(ERROR) << "Write response to call_data failed";
        return;
      }
    } else {
      call_data->write_and_finish(redirect_cntl.response_attachment().to_string());
    }

    return;
  });
}

} // namespace xllm_service
