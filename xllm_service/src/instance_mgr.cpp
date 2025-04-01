#include <absl/strings/str_join.h>
#include <chrono>
#include <glog/logging.h>

#include "instance_mgr.h"
#include "utils.h"

#include <iostream>
namespace xllm_service {

// magic number, TODO: move to config file or env var
static constexpr int kDetectIntervals = 15; // 15seconds

InstanceMgr::InstanceMgr() {
  heartbeat_thread_ =
      std::make_unique<std::thread>(&InstanceMgr::detect_disconnected_instances, this);
}

InstanceMgr::~InstanceMgr() {
  exited_ = true;
  if (heartbeat_thread_) {
    heartbeat_thread_->join();
  }
}

void InstanceMgr::detect_disconnected_instances() {
  while (!exited_) {
    std::this_thread::sleep_for(std::chrono::seconds(kDetectIntervals));
    {
      std::lock_guard<std::mutex> guard(inst_mutex_);
      auto now = std::chrono::system_clock::now();
      auto timestamp_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
      std::vector<std::string> disconnected_instances_name;
      for (const auto& [name, info] : instances_) {
        if (timestamp_ms - info.latest_timestamp > kDetectIntervals * 1000) {
          LOG(WARNING) << "Instance maybe disconnected, instance_name: " << name
                       << ", last heartbeat interval(s): "
                       << (timestamp_ms - info.latest_timestamp)/1000.0;
          disconnected_instances_name.emplace_back(name);
        }
      }
      if (utils::enable_debug_log()) {
        const auto instance_names = absl::StrJoin(disconnected_instances_name, ", ");
        LOG(INFO) << "Detect disconnected instance, instance_name: " << instance_names;
      }
      for (const auto& name : disconnected_instances_name) {
        instances_.erase(name);
      }
    }
  }
}

ErrorCode InstanceMgr::register_instance(const std::string& instance_name) {
  std::lock_guard<std::mutex> guard(inst_mutex_);
  if (utils::enable_debug_log()) {
    LOG(INFO) << "Register instance, instance_name: " << instance_name;
  }
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
  std::lock_guard<std::mutex> guard(inst_mutex_);
  if (utils::enable_debug_log()) {
    LOG(INFO) << "Register instance, instance_name: " << instance_name;
  }
  if (instances_.find(instance_name) != instances_.end()) {
    LOG(ERROR) << "Instance is already registered, instance_name: " << instance_name;
    return ErrorCode::INSTANCE_EXISTED;
  }

  instances_[instance_name] = metainfo;
  return ErrorCode::OK;
}

ErrorCode InstanceMgr::update_instance_metainfo(const std::string& instance_name,
                                                const InstanceMetaInfo& metainfo) {
  std::lock_guard<std::mutex> guard(inst_mutex_);
  if (utils::enable_debug_log()) {
    LOG(INFO) << "Update instance metainfo, instance_name: " << instance_name;
  }
  if (instances_.find(instance_name) == instances_.end()) {
    LOG(ERROR) << "Instance is not registered, instance_name: " << instance_name;
    return ErrorCode::INSTANCE_EXISTED; 
  }

  instances_[instance_name] = metainfo;
  return ErrorCode::OK;
}

ErrorCode InstanceMgr::heartbeat(const std::string& instance_name) {
  std::lock_guard<std::mutex> guard(inst_mutex_);
  if (utils::enable_debug_log()) {
    LOG(INFO) << "Receive heartbeat, instance_name: " << instance_name;
  }
  if (instances_.find(instance_name) == instances_.end()) {
    LOG(ERROR) << "Instance is not registered, instance_name: " << instance_name;
    return ErrorCode::INSTANCE_EXISTED; 
  }

  auto now = std::chrono::system_clock::now();
  auto timestamp_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
  instances_[instance_name].latest_timestamp = timestamp_ms;

  return ErrorCode::OK;
}

} // xllm_service
