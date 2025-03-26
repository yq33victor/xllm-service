include(cc_binary)

cc_binary(
  NAME 
    xllm_client
  SRCS 
    client.cpp
  DEPS
    :xllm_service
    gflags::gflags
    glog::glog
)
