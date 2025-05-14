#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <brpc/controller.h>
#include <glog/logging.h>
#include <nlohmann/json.hpp>

#include "chat.pb.h"
#include "common/call_data.h"
#include "common/closure_guard.h"
#include "completion.pb.h"
#include "http_service/service.h"
#include "rpc_service/service.h"
#include "xllm/uuid.h"

namespace xllm_service {

namespace {
thread_local llm::ShortUUID short_uuid;
std::string generate_service_request_id(const std::string& method) {
  std::stringstream ss;
  ss << method << "-";
  ss << std::this_thread::get_id();
  ss << "-";
  ss << short_uuid.random();
  return ss.str();
}
} //namespace

XllmHttpServiceImpl::XllmHttpServiceImpl(std::shared_ptr<XllmRpcServiceImpl> rpc_service,
                                         const HttpServiceConfig& config)
    : config_(config), rpc_service_(rpc_service) {
  initialized_ = true;
  thread_pool_ = std::make_unique<ThreadPool>(config_.num_threads);
}

XllmHttpServiceImpl::XllmHttpServiceImpl(const HttpServiceConfig& config)
    : config_(config) {
  initialized_ = true;
  thread_pool_ = std::make_unique<ThreadPool>(config_.num_threads);
}

XllmHttpServiceImpl::~XllmHttpServiceImpl() {
}

bool XllmHttpServiceImpl::create_channel(const std::string& target_uri) {
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
      return false;
    }
    cached_channels_[target_uri] = channel;
  }

  return true;
}

std::string XllmHttpServiceImpl::get_redirect_uri(bool only_prefill) {
  std::string target_instance_addr;
  if (!rpc_service_) {
    // for testing
    if (config_.test_instance_addr.empty()) {
      LOG(ERROR) << "Rpc service is not start.";
      return "";
    }
    target_instance_addr = config_.test_instance_addr;
  } else {
    InstancesPair instances_pair =
        rpc_service_->select_instances_pair(only_prefill);
    if (instances_pair.prefill_instance_http_addr.empty()) {
      LOG(ERROR) << "No prefill instance available.";
      return "";
    }
    target_instance_addr = instances_pair.prefill_instance_http_addr;

    if (!only_prefill) {
      if (instances_pair.decode_instance_http_addr.empty()) {
        // TODO:
      }
      // TODO: add instances_pair.decode_instance_http_addr to request?
    }
  }

  return target_instance_addr;
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

void XllmHttpServiceImpl::handle_v1_completions(std::shared_ptr<CompletionCallData> call_data,
                                                const std::string& req_attachment,
                                                const std::string& service_request_id,
                                                bool stream,
                                                const std::string& model,
                                                bool include_usage,
                                                const std::string& target_uri) {
  bool success = rpc_service_->record_new_request(call_data, service_request_id, stream,
                                                  model, include_usage);
  if (!success) {
    LOG(ERROR) << "rpc service add new request error: " << service_request_id;
    call_data->finish_with_error("Internal runtime error.");
    rpc_service_->finish_request(service_request_id);
    return;
  }

  // async redistribute the request and wait the response
  // TODO: optimize the thread pool to async mode.
  auto channel_ptr = cached_channels_[target_uri];
  // send request to prefill instance.
  thread_pool_->schedule([this, service_request_id, stream, req_attachment=std::move(req_attachment),
                          call_data, channel_ptr, target_uri=target_uri+"/v1/completions"]() {
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
      rpc_service_->finish_request(service_request_id);
      return;
    }

    // TODO: consider finish status here? like only decode one token.
    //
    if (stream) {
      if (!call_data->write(redirect_cntl.response_attachment().to_string())) {
        LOG(ERROR) << "Write response to call_data failed";
        rpc_service_->finish_request(service_request_id);
      }
    }
    // non-stream, results will be sent by decode instance.
  });
}

void XllmHttpServiceImpl::handle_v1_chat_completions(std::shared_ptr<ChatCallData> call_data,
                                                     const std::string& req_attachment,
                                                     const std::string& service_request_id,
                                                     bool stream,
                                                     const std::string& model,
                                                     bool include_usage,
                                                     const std::string& target_uri) {
  bool success = rpc_service_->record_new_request(call_data, service_request_id, stream,
                                                  model, include_usage);
  if (!success) {
    LOG(ERROR) << "rpc service add new request error: " << service_request_id;
    call_data->finish_with_error("Internal runtime error.");
    rpc_service_->finish_request(service_request_id);
    return;
  }

  // async redistribute the request and wait the response
  // TODO: optimize the thread pool to async mode.
  auto channel_ptr = cached_channels_[target_uri];
  // send request to prefill instance.
  thread_pool_->schedule([this, service_request_id, stream, req_attachment=std::move(req_attachment),
                          call_data, channel_ptr, target_uri=target_uri+"/v1/chat/completions"]() {
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
      rpc_service_->finish_request(service_request_id);
      return;
    }

    // TODO: consider finish status here? like only decode one token.
    //
    if (stream) {
      if (!call_data->write(redirect_cntl.response_attachment().to_string())) {
        LOG(ERROR) << "Write response to call_data failed";
        rpc_service_->finish_request(service_request_id);
      }
    }
    // non-stream, results will be sent by decode instance.
  });
}

void XllmHttpServiceImpl::post_serving(const std::string& serving_method,
                                       ::google::protobuf::RpcController* controller,
                                       const proto::HttpRequest* request,
                                       proto::HttpResponse* response,
                                       ::google::protobuf::Closure* done) {
  assert(initialized_);
  ClosureGuard done_guard(done);
  auto cntl = reinterpret_cast<brpc::Controller*>(controller);

  if (!request || !response || !controller) {
    LOG(ERROR) << "brpc request | respose | controller is null";
    cntl->SetFailed("brpc request | respose | controller is null");
    return;
  }

  nlohmann::json json_value;
  try {
    json_value = nlohmann::json::parse(cntl->request_attachment().to_string());
  } catch (const std::exception& e) {
    std::stringstream ss;
    ss << "Json parse request failed, error: " << e.what() << std::endl;
    LOG(ERROR) << ss.str();
    cntl->SetFailed(ss.str());
    return;
  }
  bool stream = false;
  if (json_value.contains("stream")) {
    try {
      stream = json_value.at("stream").get<bool>();
    } catch  (const std::exception& e) {
      LOG(ERROR) << "Invalid args(stream) type in request, required bool type value.";
      cntl->SetFailed("Invalid args(stream) type in request, required bool type value.");
      return;
    }
  }
  std::string model = json_value.at("model").get<std::string>();
  // TODO: add `include_usage`, `created_time` fileds etc.
  // create xllm_service request_id: service_request_id
  std::string service_request_id = generate_service_request_id(serving_method);
  json_value["service_request_id"] = service_request_id;
  std::string req_attachment = json_value.dump();

  // redistribute the request to the correct P/D instance
  // TODO: redistribute policy to select the instance
  std::string target_uri = get_redirect_uri();
  if (target_uri.empty()) {
    cntl->SetFailed("Internal runtime error, can not found a running instance.");
    return;
  }
  if (cached_channels_.find(target_uri) == cached_channels_.end()) {
    if(!create_channel(target_uri)) {
      LOG(ERROR) << "Create channel failed, target_uri is " << target_uri;
      cntl->SetFailed("Internal runtime error.");
      return;
    }
  }

  if (serving_method == "/v1/completions") {
    auto arena = response->GetArena();
    auto resp_pb =
        google::protobuf::Arena::CreateMessage<llm::proto::CompletionResponse>(arena);
    auto call_data = std::make_shared<CompletionCallData>(cntl, stream, done_guard.release(), resp_pb);
    handle_v1_completions(call_data, req_attachment, service_request_id,
                          stream, model, false/*include_usage*/, target_uri);
  } else if (serving_method == "/v1/chat/completions") {
    auto arena = response->GetArena();
    auto resp_pb =
        google::protobuf::Arena::CreateMessage<llm::proto::ChatResponse>(arena);
    auto call_data = std::make_shared<ChatCallData>(cntl, stream, done_guard.release(), resp_pb);
    handle_v1_chat_completions(call_data, req_attachment, service_request_id,
                               stream, model, false/*include_usage*/, target_uri);
  } else {
    LOG(ERROR) << "Not supported method: " << serving_method;
    cntl->SetFailed("Not supported method: " + serving_method);
    return;
  }
}

void XllmHttpServiceImpl::get_serving(const std::string& serving_method,
                                      ::google::protobuf::RpcController* controller,
                                      const proto::HttpRequest* request,
                                      proto::HttpResponse* response,
                                      ::google::protobuf::Closure* done) {
  assert(initialized_);
  ClosureGuard done_guard(done);
  auto cntl = reinterpret_cast<brpc::Controller*>(controller);

  if (!request || !response || !controller) {
    LOG(ERROR) << "brpc request | respose | controller is null";
    cntl->SetFailed("brpc request | respose | controller is null");
    return;
  }

  //auto call_data = std::make_shared<StreamCallData>(cntl, false, done_guard.release());
  auto call_data = std::make_shared<CompletionCallData>(cntl, false, done_guard.release(), nullptr);
  std::string target_uri = get_redirect_uri(true/*only_prefill*/);
  if (target_uri.empty()) {
    cntl->SetFailed("Internal runtime error, can not found a running instance.");
    return;
  }
  if (cached_channels_.find(target_uri) == cached_channels_.end()) {
    if(!create_channel(target_uri)) {
      LOG(ERROR) << "Create channel failed, target_uri is " << target_uri;
      cntl->SetFailed("Internal runtime error.");
      return;
    }
  }

  auto channel_ptr = cached_channels_[target_uri];
  target_uri += serving_method;
  thread_pool_->schedule([/*req_attachment, */call_data, cntl, channel_ptr, target_uri]() {
    brpc::Controller redirect_cntl;
    redirect_cntl.http_request().uri() = target_uri.c_str();
    redirect_cntl.http_request().set_method(brpc::HTTP_METHOD_GET);

    // Because `done'(last parameter) is NULL, this function waits until
    // the response comes back or error occurs(including timeout).
    channel_ptr->CallMethod(NULL, &redirect_cntl, NULL, NULL, NULL);
    if (redirect_cntl.Failed()) {
      LOG(ERROR) << "Redirect to instance error: " << redirect_cntl.ErrorText();
      call_data->finish_with_error(redirect_cntl.ErrorText());
      return;
    }
    call_data->write_and_finish(redirect_cntl.response_attachment().to_string());
    return;
  });
}

void XllmHttpServiceImpl::Completions(::google::protobuf::RpcController* controller,
  const proto::HttpRequest* request,
  proto::HttpResponse* response,
  ::google::protobuf::Closure* done) {
  post_serving("/v1/completions", controller, request, response, done);
}

void XllmHttpServiceImpl::ChatCompletions(::google::protobuf::RpcController* controller,
                                          const proto::HttpRequest* request,
                                          proto::HttpResponse* response,
                                          ::google::protobuf::Closure* done) {
  post_serving("/v1/chat/completions", controller, request, response, done);
}

void XllmHttpServiceImpl::Embeddings(::google::protobuf::RpcController* controller,
                                     const proto::HttpRequest* request,
                                     proto::HttpResponse* response,
                                     ::google::protobuf::Closure* done) {
  post_serving("/v1/embeddings", controller, request, response, done);
}

void XllmHttpServiceImpl::Models(::google::protobuf::RpcController* controller,
                                 const proto::HttpRequest* request,
                                 proto::HttpResponse* response,
                                 ::google::protobuf::Closure* done) {
  get_serving("/v1/models", controller, request, response, done);
}

void XllmHttpServiceImpl::Metrics(::google::protobuf::RpcController* controller,
                                  const proto::HttpRequest* request,
                                  proto::HttpResponse* response,
                                  ::google::protobuf::Closure* done) {
  get_serving("/metrics", controller, request, response, done);
}

} // namespace xllm_service
