#pragma once

#include <unordered_map>
#include <unordered_set>

#include "types.h"

namespace xllm_service {

class InstanceMgr {
 public:
  explicit InstanceMgr();
  ~InstanceMgr();

  ErrorCode register_instance(const std::string& instance_name);
  ErrorCode register_instance(const std::string& instance_name,
                              const InstanceMetaInfo& metainfo);
  ErrorCode update_instance_metainfo(const std::string& instance_name,
                                     const InstanceMetaInfo& metainfo);

 private:
  std::unordered_map<std::string, InstanceMetaInfo> instances_;
};

} // xllm_service