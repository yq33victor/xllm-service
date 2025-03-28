include(cc_binary)

cc_binary(
  NAME 
    rpc_service_test
  SRCS 
    service_test.cpp
  DEPS
    :xllm_service
    gflags::gflags
    glog::glog
    GTest::gtest_main
)

add_test(NAME RpcServiceTest COMMAND rpc_service_test)
