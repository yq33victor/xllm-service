include(cc_binary)

cc_binary(
  NAME 
    hello_client
  SRCS 
    hello_client.cpp
  DEPS
    :xllm_service
    gflags::gflags
    glog::glog
)

cc_binary(
  NAME 
    client_test
  SRCS 
    client_test.cpp
  DEPS
    :xllm_client
    gflags::gflags
    glog::glog
)

cc_binary(
  NAME 
    brpc_http_client_test
  SRCS 
    brpc_http_client.cpp
  DEPS
    brpc
    gflags::gflags
)
