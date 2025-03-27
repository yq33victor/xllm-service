include(cc_binary)
include(cc_library)

add_subdirectory(proto)

# ---- rpc service ----

cc_library(
  NAME
  xllm_service
  HDRS
    include/rpc_service/service.h
    include/utils.h
  SRCS
    src/rpc_service/service.cpp
    src/utils.cpp
  DEPS
    glog::glog
    grpc_proto::proto_service
)

cc_binary(
  NAME 
    xllm_serving
  SRCS 
    src/rpc_service/main.cpp
  DEPS
    :xllm_service
    gflags::gflags   
)

# ---- http service ----

cc_library(
  NAME
  xllm_http_service
  HDRS
    include/http_service/service.h
  SRCS
    src/http_service/service.cpp
  DEPS
    glog::glog
    proto::proto_http_service
)
target_link_libraries(xllm_http_service PRIVATE brpc leveldb::leveldb ZLIB::ZLIB protobuf::libprotobuf OpenSSL::SSL OpenSSL::Crypto)

cc_binary(
  NAME 
    xllm_http_serving
  SRCS 
    src/http_service/main.cpp
  DEPS
    :xllm_http_service
    gflags::gflags
)

add_subdirectory(examples)
