#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/sync_stream.h>

#include <iostream>
#include <memory>
#include <string>

#include "xllm_service.grpc.pb.h"

DEFINE_string(server_address, "localhost:9999", "Grpc server address.");

namespace xllm_service {

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
class HelloClient final {
 public:
  HelloClient(std::shared_ptr<Channel> channel)
      : stub_(proto::XllmService::NewStub(channel)) {}

  void hello() {
    // Create a message to send to the server
    proto::Empty request;
    proto::Status response;

    // Create a stream for receiving messages
    grpc::ClientContext ctx;
    auto status = stub_->Hello(&ctx, request, &response);
    if (!status.ok()) {
      std::cout << "Send to server faild, err msg:"
                << status.error_message() << "\n";
    }
    std::cout << "Get server response: " << response.ok() << "\n";
  }

 private:
  std::unique_ptr<proto::XllmService::Stub> stub_;
};

}  // xllm_service


int main(int argc, char* argv[]) {
  // initialize glog and gflags
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Define the server address and port
  std::string server_address(FLAGS_server_address);

  // Create a gRPC channel
  auto channel =
      grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());

  // Create a chat client
  xllm_service::HelloClient client(channel);

  client.hello();

  return 0;
}
