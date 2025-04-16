#include <csignal>

#include "common/types.h"
#include "common/utils.h"
#include "master.h"

namespace xllm_service {

Master::Master(const ServerOptions& server_options)
    : server_options_(server_options) {
  if (server_options.etcd_addr.empty()) {
    LOG(WARNING) << "etcd_addr is empty, rpc service will not save metadata to etcd.";
  }
  RpcServiceConfig rpc_config;
  rpc_config.etcd_addr = server_options.etcd_addr;
  rpc_config.disagg_pd_policy = server_options.disagg_pd_policy;
  rpc_config.detect_disconnected_instance_interval =
      server_options.detect_disconnected_instance_interval;
  rpc_service_impl_ =
      std::make_shared<xllm_service::XllmRpcServiceImpl>(rpc_config);
  rpc_service_ =
      std::make_unique<xllm_service::XllmRpcService>(rpc_service_impl_);

  HttpServiceConfig http_config;
  http_config.num_threads = server_options.http_num_threads;
  http_service_ =
      std::make_unique<xllm_service::XllmHttpServiceImpl>(rpc_service_impl_, http_config);
}

Master::~Master() {
  stop();
}

bool Master::start() {
  // 1. start http server
  http_server_thread_ = std::make_unique<std::thread>([this]() {
    start_http_server();
  });

  // 2. start rpc server
  rpc_server_thread_ = std::make_unique<std::thread>([this]() {
    start_rpc_server();
  });

  return true;
}

void Master::stop() {
  if (http_server_thread_ &&
      http_server_thread_->joinable()) {
    http_server_thread_->join();
  }

  if (rpc_server_thread_ &&
    rpc_server_thread_->joinable()) {
    rpc_server_thread_->join();
  }
}

bool Master::start_http_server() {
  if (http_server_.AddService(http_service_.get(),
                              brpc::SERVER_DOESNT_OWN_SERVICE,
                              "/hello => Hello,"
                              "/v1/completions => Completions,") != 0) {
    LOG(FATAL) << "Fail to add http service";
    return false;
  }

  brpc::ServerOptions options;
  options.idle_timeout_sec = server_options_.http_idle_timeout_s;
  options.num_threads = server_options_.http_num_threads;
  options.max_concurrency = server_options_.http_max_concurrency;

  butil::EndPoint endpoint;
  if (!server_options_.http_server_host.empty()) {
    http_server_address_ =
        server_options_.http_server_host + ":" + std::to_string(server_options_.http_port);
    if (butil::str2endpoint(http_server_address_.c_str(), &endpoint) < 0) {
        LOG(FATAL) << "Convert server_addr to endpoint failed: " << http_server_address_;
        return false;
    }
  } else {
    endpoint = butil::EndPoint(butil::IP_ANY, server_options_.http_port);
  }

  if (http_server_.Start(endpoint, &options) != 0) {
    LOG(FATAL) << "Failed to start http server on: " << endpoint;
    return false;
  }

  LOG(INFO) << "Xllm http server started on: " << endpoint;

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  http_server_.RunUntilAskedToQuit();
  return true;
}

bool Master::start_rpc_server() {
  if (rpc_server_.AddService(rpc_service_.get(),
                             brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(FATAL) << "Failed to add rpc service.";
    return false;
  }

  brpc::ServerOptions options;
  options.idle_timeout_sec = server_options_.rpc_idle_timeout_s;
  options.num_threads = server_options_.rpc_num_threads;
  options.max_concurrency = server_options_.rpc_max_concurrency;

  butil::EndPoint endpoint;
  if (!server_options_.rpc_server_host.empty()) {
    rpc_server_address_ =
        server_options_.rpc_server_host + ":" + std::to_string(server_options_.rpc_port);
    if (butil::str2endpoint(rpc_server_address_.c_str(), &endpoint) < 0) {
        LOG(FATAL) << "Convert server_addr to endpoint failed: " << rpc_server_address_;
        return false;
    }
  } else {
    endpoint = butil::EndPoint(butil::IP_ANY, server_options_.rpc_port);
  }

  if (rpc_server_.Start(endpoint, &options) != 0) {
    LOG(FATAL) << "Failed to start rpc server on: " << endpoint;
    return false;
  }

  LOG(INFO) << "Xllm rpc server started on: " << endpoint;

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  rpc_server_.RunUntilAskedToQuit();
  return true;
}

} // namespace xllm_service

DEFINE_string(http_server_host, "", "Http server listen address, may be IPV4/IPV6/UDS."
              " If this is set, the flag port will be ignored");
DEFINE_int32(http_server_port, 8888, "Port for xllm http service to listen on");
DEFINE_int32(http_server_idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(http_server_num_threads, 32, "Maximum number of threads to use");
DEFINE_int32(http_server_max_concurrency, 128, "Limit number of requests processed in parallel");

DEFINE_string(rpc_server_host, "", "Rpc server listen address, may be IPV4/IPV6/UDS."
              " If this is set, the flag port will be ignored");
DEFINE_int32(rpc_server_port, 8889, "Port for xllm rpc service to listen on");
DEFINE_int32(rpc_server_idle_timeout_s, -1, "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(rpc_server_num_threads, 32, "Maximum number of threads to use");
DEFINE_int32(rpc_server_max_concurrency, 128, "Limit number of requests processed in parallel");
DEFINE_string(etcd_addr, "0.0.0.0:2379", "etcd adderss for save instance meta info");
DEFINE_string(disagg_pd_policy, "RR", "Disaggregated prefill-decode policy.");
DEFINE_int32(detect_disconnected_instance_interval, 15,
             "The interval that server detect the disconnected instance.");

static std::atomic<uint32_t> g_signal_received{0};
void shutdown_handler(int signal) {
  LOG(WARNING) << "Received signal " << signal << ", stopping master...";
  exit(1);
}

int main(int argc, char* argv[]) {
  // Initialize gflags
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Initialize glog
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = true;

  LOG(INFO) << "Starting xllm master service.";

  // check port available or not
  if (!xllm_service::utils::is_port_available(FLAGS_http_server_port)) {
    LOG(ERROR) << "Http server port " << FLAGS_http_server_port << " is already in use. "
               << "Please specify a different port using --http_server_port flag.";
    return -1;
  }
  if (!xllm_service::utils::is_port_available(FLAGS_rpc_server_port)) {
    LOG(ERROR) << "Rpc server port " << FLAGS_rpc_server_port << " is already in use. "
               << "Please specify a different port using --rpc_server_port flag.";
    return -1;
  }

  xllm_service::ServerOptions server_options;
  server_options.http_server_host = FLAGS_http_server_host;
  server_options.http_port = FLAGS_http_server_port;
  server_options.http_idle_timeout_s = FLAGS_http_server_idle_timeout_s;
  server_options.http_num_threads = FLAGS_http_server_num_threads;
  server_options.http_max_concurrency = FLAGS_http_server_max_concurrency;
  server_options.rpc_server_host = FLAGS_rpc_server_host;
  server_options.rpc_port = FLAGS_rpc_server_port;
  server_options.rpc_idle_timeout_s = FLAGS_rpc_server_idle_timeout_s;
  server_options.rpc_num_threads = FLAGS_rpc_server_num_threads;
  server_options.rpc_max_concurrency = FLAGS_rpc_server_max_concurrency;
  server_options.etcd_addr = FLAGS_etcd_addr;
  server_options.disagg_pd_policy = FLAGS_disagg_pd_policy;
  server_options.detect_disconnected_instance_interval =
      FLAGS_detect_disconnected_instance_interval;

  xllm_service::Master master(server_options);

  if (!master.start()) {
    LOG(ERROR) << "Failed to start master service.";
    return -1;
  }

  // install graceful shutdown handler
  (void)signal(SIGINT, shutdown_handler);
  (void)signal(SIGTERM, shutdown_handler);

  while (g_signal_received.load(std::memory_order_relaxed) == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  // wait here
  master.stop();

  return 0;
}
