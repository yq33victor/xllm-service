#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "disagg_pd_policy.h"
#include "etcd_client.h"
#include "types.h"

namespace xllm_service {

class InstanceMgr {
 public:
  explicit InstanceMgr(const std::string& etcd_addr);
  ~InstanceMgr();
  ErrorCode heartbeat(const std::string& instance_name);

  ErrorCode register_instance(const std::string& instance_name);
  ErrorCode register_instance(const std::string& instance_name,
                              const InstanceMetaInfo& metainfo);
  ErrorCode update_instance_metainfo(const std::string& instance_name,
                                     const InstanceMetaInfo& metainfo);
 private:
  void internal_init();
  // save instance metainfo to etcd
  void save_persistence_metainfo(const InstanceMetaInfo& metainfo);
  // delete instance metainfo from etcd
  void delete_persistence_metainfo(const std::vector<std::string>& instance_names);
  void detect_disconnected_instances();

 private:
  bool exited_ = false;
  std::mutex inst_mutex_;
  std::unordered_map<std::string, InstanceMetaInfo> instances_;
  std::unique_ptr<std::thread> heartbeat_thread_;

  std::unique_ptr<DisaggPdPolicy> disagg_pd_policy_;

  bool use_etcd_ = false;
  std::unique_ptr<EtcdClient> etcd_client_;
};

} // namespace xllm_service
