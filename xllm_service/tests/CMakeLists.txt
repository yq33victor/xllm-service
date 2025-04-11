include(cc_binary)

cc_binary(
  NAME 
    xllm_service_test
  SRCS 
    service_test.cpp
  DEPS
    :xllm_service
    gflags::gflags
    glog::glog
    GTest::gtest_main
)

add_test(NAME XllmServiceTest COMMAND xllm_service_test)
