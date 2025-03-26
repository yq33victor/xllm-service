#include <gflags/gflags.h>
#include <glog/logging.h>
#include <grpcpp/grpcpp.h>

#include "service.h"
#include "utils.h"

// Define command line flags
DEFINE_int32(port, 9999, "Port for xllm service to listen on");
DEFINE_int32(max_threads, 4, "Maximum number of threads to use");

int main(int argc, char* argv[]) {
    // Initialize gflags
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Initialize glog
    google::InitGoogleLogging(argv[0]);
    FLAGS_logtostderr = true;

    LOG(INFO) << "Starting xllm  Service, port: " << FLAGS_port;

    if (!xllm_service::utils::is_port_available(FLAGS_port)) {
      LOG(ERROR) << "Port " << FLAGS_port << " is already in use. "
                 << "Please specify a different port using --port flag.";
      return 1;
    }

    // create xllm service
    auto xllm_service =
        std::make_shared<xllm_service::XllmService>();

    // Initialize gRPC server
    std::string server_address = "0.0.0.0:" + std::to_string(FLAGS_port);
    grpc::ServerBuilder builder;

    // Add listening port
    builder.AddListeningPort(server_address,
                             grpc::InsecureServerCredentials());

    // Register service
    xllm_service::XllmServiceImpl service(xllm_service);
    builder.RegisterService(&service);

    // Set thread pool size
    builder.SetSyncServerOption(
        grpc::ServerBuilder::SyncServerOption::NUM_CQS, FLAGS_max_threads);

    // Build and start server
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        LOG(ERROR) << "Failed to start server on port " << FLAGS_port;
        return 1;
    }

    LOG(INFO) << "Xllm service listening on " << server_address;

    // Wait for server to shutdown
    server->Wait();

    return 0;
}
