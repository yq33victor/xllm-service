include(cc_binary)
include(cc_library)

add_subdirectory(proto)

cc_library(
  NAME
  xllm_service
  HDRS
    include/service.h
    include/utils.h
  SRCS
    src/service.cpp
    src/utils.cpp
  DEPS
    glog::glog
    grpc_proto::proto_service
)

cc_binary(
  NAME 
    xllm_serving
  SRCS 
    src/main.cpp
  DEPS
    :xllm_service
    gflags::gflags   
)

add_subdirectory(examples)
