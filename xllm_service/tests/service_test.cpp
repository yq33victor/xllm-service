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
      std::make_shared<XllmServiceImpl>("");
  std::string inst_name = "127.0.0.1@nic0";
  InstanceMetaInfo metainfo(inst_name, InstanceType::PREFILL);
  EXPECT_EQ(ErrorCode::OK, xllm_service->register_instance(inst_name, metainfo));

  metainfo.type = InstanceType::DECODE;
  EXPECT_EQ(ErrorCode::INSTANCE_EXISTED, xllm_service->register_instance(inst_name, metainfo));
}

TEST_F(XllmServiceTest, UpdateInstanceMetainfo) {
  auto xllm_service =
      std::make_shared<XllmServiceImpl>("");
  std::string inst_name = "127.0.0.1@nic0";
  InstanceMetaInfo metainfo(inst_name, InstanceType::PREFILL);
  EXPECT_EQ(ErrorCode::OK, xllm_service->register_instance(inst_name, metainfo));
  metainfo.type = InstanceType::DECODE;
  EXPECT_EQ(ErrorCode::OK, xllm_service->update_instance_metainfo(inst_name, metainfo));

  std::string inst_name2 = "127.0.0.1@nic2";
  InstanceMetaInfo metainfo2(inst_name2, InstanceType::PREFILL);
  EXPECT_EQ(ErrorCode::INSTANCE_NOT_EXISTED, xllm_service->update_instance_metainfo(inst_name2, metainfo));
}

} // namespace xllm_service::test
