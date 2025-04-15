include(cc_binary)
include(cc_library)

add_subdirectory(proto)

# ---- rpc service ----

cc_library(
  NAME
  xllm_rpc_service
  HDRS
    include/common/utils.h
    include/rpc_service/service.h
    include/instance_mgr.h
    include/disagg_pd_policy.h
    include/etcd_client.h
    include/types.h
  SRCS
    src/common/utils.cpp
    src/rpc_service/service.cpp
    src/disagg_pd_policy.cpp
    src/instance_mgr.cpp
    src/etcd_client.cpp
  DEPS
    absl::strings
    brpc-static
    cpprest
    etcd-cpp-api
    glog::glog
    proto::proto_rpc_service
    nlohmann_json::nlohmann_json
)

cc_binary(
  NAME 
    xllm_rpc_serving
  SRCS 
    src/rpc_service/main.cpp
  DEPS
    :xllm_rpc_service
    gflags::gflags   
)

cc_library(
  NAME
  xllm_rpc_client
  HDRS
    include/rpc_service/client.h
    include/types.h
  SRCS
    src/rpc_service/client.cpp
  DEPS
    brpc-static
    glog::glog
    proto::proto_rpc_service
)

# ---- http service ----

cc_library(
  NAME
  xllm_http_service
  HDRS
    include/http_service/service.h
    include/common/concurrent_queue.h
    include/common/threadpool.h
  SRCS
    src/http_service/service.cpp
    src/common/threadpool.cpp
  DEPS
    :xllm_rpc_service
    absl::synchronization
    glog::glog
    nlohmann_json::nlohmann_json
    proto::proto_http_service
)
target_link_libraries(xllm_http_service PRIVATE brpc-static leveldb::leveldb ZLIB::ZLIB protobuf::libprotobuf OpenSSL::SSL OpenSSL::Crypto)

cc_binary(
  NAME 
    xllm_http_serving
  SRCS 
    src/http_service/main.cpp
  DEPS
    :xllm_http_service
    gflags::gflags
)

# ---- master service ----

cc_binary(
  NAME
    xllm_master_serving
  HDRS
    include/master.h
  SRCS
    src/master.cpp
  DEPS
    :xllm_http_service
    :xllm_rpc_service
)

add_subdirectory(examples)
add_subdirectory(tests)
