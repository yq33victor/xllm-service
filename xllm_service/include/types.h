#pragma once

#include <cstdint>
#include <string>

namespace xllm_service {

enum class ErrorCode : int32_t {
  OK = 0,
  INTERNAL_ERROR = 1,
  INSTANCE_EXISTED = 2,
};

class ConvertErrorCode {
  public:
   static int32_t to_int(ErrorCode code) noexcept {
    return static_cast<int32_t>(code);
   }

   static ErrorCode from_int(int32_t code) noexcept {
    return static_cast<ErrorCode>(code);
   }
};

enum class InstanceType : int8_t {
  DEFAULT = 0,
  // prefill instance
  PREFILL = 1,
  // decode instance
  DECODE = 2,
};

struct InstanceMetaInfo {
 public:
  InstanceMetaInfo() {}
  InstanceMetaInfo(const std::string& inst_name)
      : name(inst_name) {}
  InstanceMetaInfo(const std::string& inst_name,
                   const InstanceType& inst_type)
      : name(inst_name), type(inst_type) {}

  std::string name = "";
  InstanceType type = InstanceType::DEFAULT;
  // TODO
};

} // xllm_service
