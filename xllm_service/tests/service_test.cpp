#include <glog/logging.h>
#include <gtest/gtest.h>

#include "rpc_service/service.h"

namespace xllm_service::test {

class XllmServiceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    google::InitGoogleLogging("XllmServiceTest");
  }

  void TearDown() override {
    google::ShutdownGoogleLogging();
  }
};

TEST_F(XllmServiceTest, RegisterInstance) {
  auto xllm_service =
      std::make_shared<XllmServiceImpl>();
  std::string inst_name = "127.0.0.1@xllm0";
  InstanceMetaInfo metainfo(inst_name, InstanceType::PREFILL);
  EXPECT_EQ(ErrorCode::OK, xllm_service->register_instance(inst_name, metainfo));

  metainfo.type = InstanceType::DECODE;
  EXPECT_EQ(ErrorCode::INSTANCE_EXISTED, xllm_service->register_instance(inst_name, metainfo));
}

} // xllm_service::test
