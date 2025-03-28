#include <glog/logging.h>

#include "instance_mgr.h"

namespace xllm_service {

InstanceMgr::InstanceMgr() {}

InstanceMgr::~InstanceMgr() {}

ErrorCode InstanceMgr::register_instance(const std::string& instance_name) {
  if (instances_.find(instance_name) != instances_.end()) {
    LOG(ERROR) << "Instance is already registered, instance_name: " << instance_name;
    return ErrorCode::INSTANCE_EXISTED; 
  }

  InstanceMetaInfo default_info(instance_name);
  instances_[instance_name] = default_info;
  return ErrorCode::OK;
}

ErrorCode InstanceMgr::register_instance(const std::string& instance_name,
                                    const InstanceMetaInfo& metainfo) {
  if (instances_.find(instance_name) != instances_.end()) {
    LOG(ERROR) << "Instance is already registered, instance_name: " << instance_name;
    return ErrorCode::INSTANCE_EXISTED;
  }

  instances_[instance_name] = metainfo;
  return ErrorCode::OK;
}

ErrorCode InstanceMgr::update_instance_metainfo(const std::string& instance_name,
                                           const InstanceMetaInfo& metainfo) {
  if (instances_.find(instance_name) == instances_.end()) {
    LOG(ERROR) << "Instance is not registered, instance_name: " << instance_name;
    return ErrorCode::INSTANCE_EXISTED; 
  }

  instances_[instance_name] = metainfo;
  return ErrorCode::OK;
}

} // xllm_service
