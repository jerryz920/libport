
#include "session.h"
#include <boost/asio.hpp>



namespace latte {
Session::Session(io_service &service, std::shared_ptr<LatteDispatcher> dispatcher): 
  s_(service), dispatcher_(dispatcher) {
    sid_ = utils::gen_rand_uint64();
    log("session %u created", sid_);
  }


bool Session::authenticate() {
  auto ids = utils::unix_auth_id(s_.native_handle());
  s_.non_blocking(true);
  pid_ = std::get<0>(ids);
  uid_ = std::get<1>(ids);
  gid_ = std::get<2>(ids);
  log("session authentication: %u %u %u", pid_, uid_, gid_);
  /// pid can't be 0
  if (pid_ == 0) {
    return false;
  }
  return true;
}

void Session::proto_start() {
  log("session %u proto starts\n", sid_);
  async_read(s_, buffer(rcv_buf_, COMM_HEADER_SZ),
      std::bind(&Session::header_received, shared_from_this(),
        std::placeholders::_1, std::placeholders::_2));
}

void Session::header_received(const boost::system::error_code &ec, size_t len) {
  if (ec) {
    log("error in receving header: %s", ec.message().c_str());
    stop();
    return ;
  }
  if (len < COMM_HEADER_SZ) {
    log("received %d for header, expect %d, abort", len, COMM_HEADER_SZ);
    stop();
    return;
  }

  uint32_t size, magic;
  CodedInputStream::ReadLittleEndian32FromArray(&rcv_buf_[0], &size);
  CodedInputStream::ReadLittleEndian32FromArray(
      &rcv_buf_[sizeof(size)], &magic);
  if (magic != PROTO_MAGIC) {
    log("warning: magic differs: %x, expect %x", magic, PROTO_MAGIC);
  }
  if (size > RECV_BUFSZ) {
    //// should have a maximum bound for the size or alternative
    //receive mechanism for super large thing. And should have a timer.
    large_rcv_buf_ = std::vector<uint8_t>(size, 0);
    async_read(s_, buffer(large_rcv_buf_, size),
        std::bind(&Session::command_received, shared_from_this(),
          std::placeholders::_1, size, &large_rcv_buf_[0]));
  } else {
    async_read(s_, buffer(rcv_buf_, size),
        std::bind(&Session::command_received, shared_from_this(),
          std::placeholders::_1, size, &rcv_buf_[0]));
  }
}

void Session::command_received(const boost::system::error_code &ec, size_t len, uint8_t *data) {
  utils::Guards<void(void)>([this, len]() { 
      if (len > RECV_BUFSZ) {
      clear_large_rcv_buffer(); 
      }
    });
  if (ec) {
    log("error in receving command: %s", ec.message().c_str());
    stop();
    return ;
  }
  auto result = std::make_shared<proto::Command>();
  if (!result->ParseFromArray(data, len)) {
    log("error parsing received command");
    stop();
    return ;
  }
  /// process should be async processing in fact.
  /// Response resp = process_cmd(command)
  result->set_pid(pid_);
  result->set_uid(uid_);
  result->set_gid(gid_);
  log("command received, auth %s, type %s", result->auth().c_str(),
      result->Type_Name(result->type()).c_str());
  dispatcher_->dispatch(result, std::bind(&Session::write_response,
        shared_from_this(), std::placeholders::_1));
}

static inline void serialize_msg(const proto::Response &resp,
    unsigned char *ptr) {
  uint32_t sz = resp.ByteSize();
  CodedOutputStream::WriteLittleEndian32ToArray(sz, ptr);
  CodedOutputStream::WriteLittleEndian32ToArray(PROTO_MAGIC,
      ptr + sizeof(uint32_t));
  resp.SerializeToArray(ptr + COMM_HEADER_SZ, sz);
}

/// callback for dispatcher
void Session::write_response(std::shared_ptr<proto::Response> resp) {
  size_t msgsz = resp->ByteSize() + COMM_HEADER_SZ;
  log("response: %u bytes, result type %s", msgsz, 
      resp->Type_Name(resp->type()).c_str());
  if (msgsz > SEND_BUFSZ) {
    large_snd_buf_ = decltype(large_snd_buf_)(msgsz, 0);
    serialize_msg(*resp, &large_snd_buf_[0]);
    async_write(s_, buffer(large_snd_buf_, msgsz),
        std::bind(&Session::response_sent, shared_from_this(),
          std::placeholders::_1, std::placeholders::_2));
  } else {
    serialize_msg(*resp, &snd_buf_[0]);
    async_write(s_, buffer(snd_buf_, msgsz),
        std::bind(&Session::response_sent, shared_from_this(),
          std::placeholders::_1, msgsz));
  }
}

void Session::response_sent(const boost::system::error_code &ec, size_t len) {
  utils::Guards<void(void)>([this, len] () {
      if (len > SEND_BUFSZ) {
      clear_large_snd_buffer();
      }
      });
  if (ec) {
    log_err("error in sending response: %s", ec.message().c_str());
    stop();
    return;
  }
  proto_start();
}

}
















