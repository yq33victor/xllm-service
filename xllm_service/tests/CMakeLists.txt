include(cc_binary)

cc_binary(
  NAME 
    xllm_rpc_service_test
  SRCS 
    rpc_service_test.cpp
  DEPS
    :xllm_rpc_service
    gflags::gflags
    glog::glog
    GTest::gtest_main
)

add_test(NAME XllmRpcServiceTest COMMAND xllm_rpc_service_test)
