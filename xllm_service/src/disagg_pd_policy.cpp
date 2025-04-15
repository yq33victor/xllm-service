#include "disagg_pd_policy.h"

namespace xllm_service {

DisaggPdPolicy::DisaggPdPolicy(const std::unordered_map<std::string, InstanceMetaInfo>* insts)
    : instances_(insts) {}

DisaggPdPolicy::~DisaggPdPolicy() {}

std::unordered_map<std::string, InstanceType>
DisaggPdPolicy::reallocate_instances_type(/*params here*/) {
  // TODO: implement this function
  return {};
}

std::unordered_map<std::string, std::vector<std::string>>
DisaggPdPolicy::allocate_pd_pairs(/*params here*/) {
  // TODO: implement this function
  return {};
}

} // namespace xllm_service