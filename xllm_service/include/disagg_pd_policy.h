#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "types.h"

namespace xllm_service {

class DisaggPdPolicy {
 public:
  DisaggPdPolicy(const std::unordered_map<std::string, InstanceMetaInfo>*);
  ~DisaggPdPolicy();

  // re-allocate instance types: prefill or decode
  std::unordered_map<std::string, InstanceType> reallocate_instances_type(/*params here*/);

  // Allocate prefill and decode pairs, return prefill -> [decode instances]
  // Allow multiple decode instances for each prefill instance and 
  // multiple prefill instances for each decode instance
  std::unordered_map<std::string, std::vector<std::string>> allocate_pd_pairs(/*params here*/);

 private:
  const std::unordered_map<std::string, InstanceMetaInfo>* instances_; // not owned
};

} // xllm_service
