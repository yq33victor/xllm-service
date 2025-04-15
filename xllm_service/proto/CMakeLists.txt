include(grpc_proto_library)
include(proto_library)

proto_library(
  NAME
    proto_service
  SRCS
    xllm_service.proto
)

proto_library(
  NAME
    proto_http_service
  SRCS
    xllm_http_service.proto
)

