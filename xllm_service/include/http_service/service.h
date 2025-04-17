#pragma once

#include <brpc/channel.h>
#include <iostream>
#include <mutex>

#include "common/types.h"
#include "common/threadpool.h"
#include "http_service/call_data.h"
#include "xllm_http_service.pb.h"

namespace xllm_service {

class XllmRpcServiceImpl;

class XllmHttpServiceImpl : public proto::XllmHttpService {
 public:
  XllmHttpServiceImpl(const HttpServiceConfig& config);
  XllmHttpServiceImpl(std::shared_ptr<XllmRpcServiceImpl> rpc_service,
                      const HttpServiceConfig& config);
  ~XllmHttpServiceImpl();

  void Hello(::google::protobuf::RpcController* controller,
             const proto::HttpHelloRequest* request,
             proto::HttpHelloResponse* response,
             ::google::protobuf::Closure* done) override;

  void Completions(::google::protobuf::RpcController* controller,
                   const proto::HttpRequest* request,
                   proto::HttpResponse* response,
                   ::google::protobuf::Closure* done) override;

  void ChatCompletions(::google::protobuf::RpcController* controller,
                       const proto::HttpRequest* request,
                       proto::HttpResponse* response,
                       ::google::protobuf::Closure* done) override;

  void Embeddings(::google::protobuf::RpcController* controller,
                  const proto::HttpRequest* request,
                  proto::HttpResponse* response,
                  ::google::protobuf::Closure* done) override;

  void Models(::google::protobuf::RpcController* controller,
              const proto::HttpRequest* request,
              proto::HttpResponse* response,
              ::google::protobuf::Closure* done) override;

  void Metrics(::google::protobuf::RpcController* controller,
               const proto::HttpRequest* request,
               proto::HttpResponse* response,
               ::google::protobuf::Closure* done) override;

 private:
  bool create_channel(const std::string& target_uri);
  // only prefill is true means only prefill instance is returned
  std::string get_redirect_uri(std::shared_ptr<StreamCallDataBrpc> call_data,
                               bool only_prefill = false);
  void post_serving(const std::string& serving_method,
                    ::google::protobuf::RpcController* controller,
                    const proto::HttpRequest* request,
                    proto::HttpResponse* response,
                    ::google::protobuf::Closure* done);
  void get_serving(const std::string& serving_method,
                   ::google::protobuf::RpcController* controller,
                   const proto::HttpRequest* request,
                   proto::HttpResponse* response,
                   ::google::protobuf::Closure* done);

 private:
  bool initialized_ = false;
  HttpServiceConfig config_;

  std::shared_ptr<XllmRpcServiceImpl> rpc_service_;

  // uri -> channel
  // e.g. 127.0.0.1:9999/v1/completions -> channel1
  //      127.0.0.1:9999/v1/chat/completions -> channel2
  // NOTE: different methods to one instance has different channels
  std::unordered_map<std::string, brpc::Channel*> cached_channels_;
  std::unique_ptr<ThreadPool> thread_pool_;
  std::mutex channel_mutex_;
};

} // namespace xllm_service
