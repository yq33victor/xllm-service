include(cc_binary)
include(cc_library)
include(cc_test)

add_subdirectory(proto)
add_subdirectory(include/huggingface)

# ---- tokenizers ----

cc_library(
  NAME 
    tokenizer
  HDRS
    include/common/json_reader.h
    include/common/macros.h
    include/common/slice.h
    include/tokenizers/tokenizer_args.h
    include/tokenizers/tokenizer.h
    include/tokenizers/tiktoken_tokenizer.h
    include/tokenizers/sentencepiece_tokenizer.h
    include/tokenizers/hf_tokenizer.h
    include/tokenizers/tokenizer_args_loader.h
  SRCS
    src/common/json_reader.cpp
    src/tokenizers/hf_tokenizer.cpp
    src/tokenizers/sentencepiece_tokenizer.cpp
    src/tokenizers/tiktoken_tokenizer.cpp
    src/tokenizers/tokenizer_args_loader.cpp
  DEPS
    sentencepiece
    absl::flat_hash_map
    absl::strings
    huggingface
    glog::glog
    re2::re2
    nlohmann_json::nlohmann_json
)

cc_test(
  NAME
    tokenizer_test
  SRCS
    src/tokenizers/sentencepiece_tokenizer_test.cpp
    src/tokenizers/tiktoken_tokenizer_test.cpp
  DEPS
    :tokenizer
    GTest::gtest_main
      src/tokenizers/data
      src/tokenizers/data/tokenizer.model
      src/tokenizers/data/test.tiktoken
)

# ---- chat template ----

cc_library (
  NAME
    chat_template
  HDRS
    include/chat_template/chat_template.h
    include/chat_template/coded_chat_template.h
    include/chat_template/common_chat_template.h
    include/chat_template/jinja_chat_template.h
  SRCS
    src/chat_template/coded_chat_template.cpp
    src/chat_template/common_chat_template.cpp
    src/chat_template/jinja_chat_template.cpp
  DEPS
    :minja
    :tokenizer
    nlohmann_json::nlohmann_json
    glog::glog
)

cc_test (
  NAME
    chat_template_test
  SRCS
    src/chat_template/jinja_chat_template_test.cpp
  DEPS
    :chat_template
    GTest::gtest_main
)


# ---- rpc service ----

cc_library(
  NAME
  xllm_rpc_service
  HDRS
    include/common/call_data.h
    include/common/global_gflags.h
    include/common/threadpool.h
    include/common/types.h
    include/common/utils.h
    include/disagg_pd_policy.h
    include/etcd_client.h
    include/instance_mgr.h
    include/rpc_service/response_handler.h
    include/rpc_service/service.h
    include/xllm/output.h
    include/xllm/status.h
  SRCS
    src/common/threadpool.cpp
    src/common/utils.cpp
    src/common/global_gflags.cpp
    src/disagg_pd_policy.cpp
    src/etcd_client.cpp
    src/instance_mgr.cpp
    src/rpc_service/response_handler.cpp
    src/rpc_service/service.cpp
  DEPS
    absl::random_random
    absl::strings
    cpprest
    etcd-cpp-api
    glog::glog
    nlohmann_json::nlohmann_json
    proto::proto_rpc_service
    proto_xllm
    tokenizer
    chat_template
)
target_link_libraries(xllm_rpc_service PRIVATE brpc-static)

cc_binary(
  NAME 
    xllm_rpc_serving
  SRCS 
    src/rpc_service/main.cpp
  DEPS
    :xllm_rpc_service
    gflags::gflags   
)

cc_library(
  NAME
  xllm_rpc_client
  HDRS
    include/common/types.h
    include/rpc_service/client.h
  SRCS
    src/rpc_service/client.cpp
  DEPS
    glog::glog
    proto::proto_rpc_service
)
target_link_libraries(xllm_rpc_client PRIVATE brpc-static)

# ---- http service ----

cc_library(
  NAME
  xllm_http_service
  HDRS
    include/common/closure_guard.h
    include/common/call_data.h
    include/common/concurrent_queue.h
    include/common/threadpool.h
    include/http_service/service.h
    include/http_service/request_tracer.h
    include/xllm/output.h
    include/xllm/status.h
    include/xllm/uuid.h
  SRCS
    src/common/threadpool.cpp
    src/http_service/service.cpp 
    src/http_service/request_tracer.cpp
    include/xllm/uuid.cpp
  DEPS
    :xllm_rpc_service
    absl::random_random
    absl::synchronization
    glog::glog
    nlohmann_json::nlohmann_json
    proto::proto_http_service
    proto_xllm
)
target_link_libraries(xllm_http_service PRIVATE brpc-static leveldb::leveldb ZLIB::ZLIB protobuf::libprotobuf OpenSSL::SSL OpenSSL::Crypto)

cc_binary(
  NAME 
    xllm_http_serving
  SRCS 
    src/http_service/main.cpp
  DEPS
    :xllm_http_service
    gflags::gflags
)

# ---- master service ----

cc_binary(
  NAME
    xllm_master_serving
  HDRS
    include/master.h
  SRCS
    src/master.cpp
  DEPS
    :xllm_http_service
    :xllm_rpc_service
)

add_subdirectory(examples)
add_subdirectory(tests)