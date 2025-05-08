#include <brpc/closure_guard.h>

#include "common/types.h"
#include "rpc_service/service.h"

namespace xllm_service {

XllmRpcServiceImpl::XllmRpcServiceImpl(const RpcServiceConfig& config) {
  instance_mgr_ = std::make_unique<InstanceMgr>(config);
}

XllmRpcServiceImpl::~XllmRpcServiceImpl() {
}

ErrorCode XllmRpcServiceImpl::heartbeat(const std::string& instance_name) {
  return instance_mgr_->heartbeat(instance_name);
}

ErrorCode XllmRpcServiceImpl::register_instance(const std::string& instance_name,
                                                const InstanceMetaInfo& metainfo) {
  return instance_mgr_->register_instance(instance_name, metainfo);
}

ErrorCode XllmRpcServiceImpl::update_instance_metainfo(const std::string& instance_name,
                                                       const InstanceMetaInfo& metainfo) {
  return instance_mgr_->update_instance_metainfo(instance_name, metainfo);
}

InstancesPair XllmRpcServiceImpl::select_instances_pair(bool only_prefill) {
  return instance_mgr_->select_instances_pair(only_prefill);
}

InstanceMetaInfo XllmRpcServiceImpl::get_instance_info(const std::string& instance_name) {
  return instance_mgr_->get_instance_info(instance_name);
}

std::vector<std::string> XllmRpcServiceImpl::get_static_decode_list(const std::string& instance_name) {
  return instance_mgr_->get_static_decode_list(instance_name);
}

XllmRpcService::XllmRpcService(std::shared_ptr<XllmRpcServiceImpl> service)
    : xllm_service_(service) {
}

XllmRpcService::~XllmRpcService() {
}

void XllmRpcService::Hello(
    google::protobuf::RpcController* cntl_base,
    const proto::Empty* req,
    proto::Status* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  resp->set_ok(true);
}

void XllmRpcService::RegisterInstance(
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
  InstanceMetaInfo metainfo(req->name(), req->rpc_address(), type);
  metainfo.cluster_id = req->cluster_id();
  metainfo.cache_ids =
      std::vector<uint64_t>(req->cache_ids().begin(),
                            req->cache_ids().end());
  for (auto& tensor_addr : req->tensor_addrs()) {
    std::vector<uint64_t> addr =
      std::vector<uint64_t>(tensor_addr.layer_addrs().begin(),
                            tensor_addr.layer_addrs().end());
    metainfo.tensor_addrs.emplace_back(std::move(addr));
  }
  ErrorCode code = xllm_service_->register_instance(req->name(), metainfo);
  resp->set_status_code(ConvertErrorCode::to_int(code));
}

void XllmRpcService::GetInstanceInfo(
    google::protobuf::RpcController* cntl_base,
    const proto::InstanceID* req,
    proto::InstanceMetaInfo* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  InstanceMetaInfo metainfo = xllm_service_->get_instance_info(req->name());
  resp->set_name(metainfo.name);
  resp->set_rpc_address(metainfo.rpc_address);
  if (metainfo.type == InstanceType::PREFILL) {
    resp->set_type(proto::InstanceType::PREFILL);
  } else if (metainfo.type == InstanceType::DECODE) {
    resp->set_type(proto::InstanceType::DECODE);
  } else {
    resp->set_type(proto::InstanceType::DEFAULT);
  }
  resp->set_cluster_id(metainfo.cluster_id);
  for (auto& cache_id : metainfo.cache_ids) {
    *(resp->mutable_cache_ids()->Add()) = cache_id;
  }
  for (auto& tensor_addr : metainfo.tensor_addrs) {
    auto proto_layer_addr = resp->mutable_tensor_addrs()->Add();
    for (auto& layer_addr : tensor_addr) {
      *(proto_layer_addr->mutable_layer_addrs()->Add()) = layer_addr;
    }
  }
}

void XllmRpcService::Heartbeat(
    google::protobuf::RpcController* cntl_base,
    const proto::HeartbeatRequest* req,
    proto::Status* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  auto inst_name = req->name();
  xllm_service_->heartbeat(inst_name);
  resp->set_ok(true);
}

void XllmRpcService::GetStaticDecodeList(
    google::protobuf::RpcController* cntl_base,
    const proto::InstanceID* req,
    proto::InstanceIDs* resp,
    google::protobuf::Closure* done) {
  brpc::ClosureGuard done_guard(done);
  std::vector<std::string> decode_list = xllm_service_->get_static_decode_list(req->name());
  for (auto& d : decode_list) {
    *(resp->mutable_names()->Add()) = std::move(d);
  }
}

} // namespace xllm_service
