#pragma once

#include <brpc/controller.h>
#include <butil/iobuf.h>
#include <glog/logging.h>
#include <string>

// Interface for the classes that are used to handle grpc requests.
class CallData {
 public:
  virtual ~CallData() = default;

  // returns true if the rpc is ok and the call data is not finished
  // returns false if the call data is finished and can be deleted
  virtual bool proceed(bool rpc_ok) = 0;

  void get_x_request_id(std::string& x_request_id, brpc::Controller* ctrl) {
    x_request_id = "";
    if (ctrl->http_request().GetHeader("x-request-id")) {
      x_request_id = *ctrl->http_request().GetHeader("x-request-id");
    } else if (ctrl->http_request().GetHeader("x-ms-client-request-id")) {
      x_request_id = *ctrl->http_request().GetHeader("x-ms-client-request-id");
    }
    return;
  }

  void get_x_request_time(std::string& x_request_time, brpc::Controller* ctrl) {
    x_request_time = "";
    if (ctrl->http_request().GetHeader("x-request-time")) {
      x_request_time = *ctrl->http_request().GetHeader("x-request-time");
    } else if (ctrl->http_request().GetHeader("x-request-timems")) {
      x_request_time = *ctrl->http_request().GetHeader("x-request-timems");
    }
    return;
  }

 public:
  std::string x_request_id;
  std::string x_request_time;
};

class StreamCallDataBrpc : public CallData {
 public:
  StreamCallDataBrpc(brpc::Controller* controller,
                     bool stream,
                     ::google::protobuf::Closure* done)
      : controller_(controller),
        done_(done) {
    stream_ = stream;
    get_x_request_id(x_request_id, controller_);
    get_x_request_time(x_request_time, controller_);
   
    if (stream_) {
      pa_ = controller_->CreateProgressiveAttachment();

      controller_->http_response().set_content_type("text/event-stream");
      controller_->http_response().set_status_code(200);
      controller_->http_response().SetHeader("Connection", "keep-alive");
      controller_->http_response().SetHeader("Cache-Control", "no-cache");
      // Done Run first for steam response
      done_->Run();

    } else {
      controller_->http_response().SetHeader("Content-Type",
                                             "text/javascript; charset=utf-8");
    }
  }

  ~StreamCallDataBrpc() {
    // For non stream response, call brpc done Run
    if (!stream_) {
      done_->Run();
    }
  }

  bool proceed(bool rpc_ok) override { return true; }

  // For non stream response
  bool write_and_finish(const std::string& attachment/*json string*/) {
    controller_->response_attachment() = attachment;
    return true;
  }

  // For non stream response
  bool finish_with_error(const std::string& error_message) {
    if (!stream_) {
      controller_->SetFailed(error_message);
    } else {
      io_buf_.clear();
      io_buf_.append(error_message);
      pa_->Write(io_buf_);
    }

    return true;
  }

  // For stream response
  bool write(const butil::IOBuf& attachment_iobuf) {
    pa_->Write(attachment_iobuf);
    return true;
  }

  // For stream response
  bool write(const std::string& attachment) {
    io_buf_.clear();
    io_buf_.append(attachment);
    pa_->Write(io_buf_);
    if (attachment.find("data: [DONE]") != std::string::npos) {
      finished_ = true;
    }

    return true;
  }

  ::google::protobuf::Closure* done() { return done_; }
  bool finished() { return finished_; }

 private:
  brpc::Controller* controller_;
  ::google::protobuf::Closure* done_;

  bool stream_ = false;
  butil::intrusive_ptr<brpc::ProgressiveAttachment> pa_;
  butil::IOBuf io_buf_;

  bool finished_ = false;
};
