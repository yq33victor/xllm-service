#pragma once

#include <chrono>
#include <cstdint>
#include <glog/logging.h>
#include <string>

namespace xllm_service {

enum class ErrorCode : int32_t {
  OK = 0,
  INTERNAL_ERROR = 1,
  INSTANCE_EXISTED = 2,
  INSTANCE_NOT_EXISTED = 3,
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
  InstanceMetaInfo() {
    set_init_timestamp();
  }
  InstanceMetaInfo(const std::string& inst_name)
      : name(inst_name) {
    set_init_timestamp();
  }
  InstanceMetaInfo(const std::string& inst_name,
                   const InstanceType& inst_type)
      : name(inst_name), type(inst_type) {
    set_init_timestamp();
  }

  std::string name = "";
  InstanceType type = InstanceType::DEFAULT;

  // latest heatbeat timestamp
  uint64_t latest_timestamp = 0;

 private:
  void set_init_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto timestamp_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    latest_timestamp = timestamp_ms;
  }
};

// the info be stored in etcd
struct InstanceIdentityInfo {
  std::string instance_addr;
  int8_t instance_type; // convert to InstanceType

  const std::string debug_string() const {
    std::string debug_str =
        "instance_addr: " + instance_addr + \
         ", instance_type: " + std::to_string((int)(instance_type));
    return debug_str;
  }
};

} // xllm_service
