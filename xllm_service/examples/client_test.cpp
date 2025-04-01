#include <gflags/gflags.h>
#include <glog/logging.h>

#include "rpc_service/client.h"

DEFINE_string(server_address, "localhost:9999", "Grpc server address.");

int main(int argc, char* argv[]) {
  // initialize glog and gflags
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Define the server address and port
  std::string server_address(FLAGS_server_address);

  // Create a client instance
  xllm_service::XllmClient client("127.0.0.1@nic0", server_address);

  // Register the instance
  auto ret = client.register_instance();
  if (ret != xllm_service::ErrorCode::OK) {
    LOG(ERROR) << "Register instance failed.";
    return -1;
  }

  // Keep the client running
  while (true) {
    sleep(1);
  }

  return 0;
}
