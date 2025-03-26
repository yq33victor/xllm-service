include(grpc_proto_library)

grpc_proto_library(
  NAME
    proto_service
  SRCS
    xllm_service.proto
)
