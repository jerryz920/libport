

#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "proto/statement.pb.h"
#include "config.h"
#include "log.h"
#include "utils.h"
#include "comm.h"
#include "dispatcher.h"

using namespace boost::asio;
using google::protobuf::io::CodedInputStream;

namespace latte {

typedef Dispatcher<proto::Command, proto::Response> LatteDispatcher;

class Session: public std::enable_shared_from_this<Session> {

  public:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Session);

    static std::shared_ptr<Session> create(
        boost::asio::io_service &service, std::shared_ptr<LatteDispatcher> dispatcher) {
      return std::make_shared<Session>(service, dispatcher);
    }

    ~Session() {
      log("session %u destoryed", sid_);
    }

    void start() {
      if (!authenticate()) {
        s_.close();
      } else {
        proto_start();
      }

    }

    void stop() {
      s_.close();
    }
    Session(io_service &service, std::shared_ptr<LatteDispatcher> dispatcher): 
      s_(service), dispatcher_(dispatcher) {
      sid_ = utils::gen_rand_uint64();
      s_.non_blocking(true);
      log("session %u created", sid_);
    }

  private:


    bool authenticate() {
      auto ids = utils::unix_auth_id(s_.native_handle());
      pid_ = std::get<0>(ids);
      uid_ = std::get<1>(ids);
      gid_ = std::get<2>(ids);
      /// pid can't be 0
      if (pid_ == 0) {
        return false;
      }
      return true;
    }

    void proto_start() {
      async_read(s_, buffer(rcv_buf_, COMM_HEADER_SZ),
          std::bind(&Session::header_received, shared_from_this(),
            std::placeholders::_1, std::placeholders::_2));
    }

    void header_received(boost::system::error_code ec, size_t len) {
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

    void command_received(boost::system::error_code ec, size_t len, uint8_t *data) {
      utils::Guards<void(void)>([this, len]() { 
          if (len > RECV_BUFSZ) {
            clear_large_rcv_buffer(); 
          }
        }
      );
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
      dispatcher_->dispatch(result, std::bind(&Session::write_response,
            shared_from_this(), std::placeholders::_1));
    }

    /// callback for dispatcher
    void write_response(std::shared_ptr<proto::Response> resp) {
      size_t sz = resp->ByteSize();
      if (sz > SEND_BUFSZ) {
        large_snd_buf_ = decltype(large_snd_buf_)(sz, 0);
        async_write(s_, buffer(large_snd_buf_, sz),
          std::bind(&Session::response_sent, shared_from_this(),
            std::placeholders::_1, std::placeholders::_2));
        resp->SerializeToArray(&large_snd_buf_[0], sz);
      } else {
        async_write(s_, buffer(snd_buf_, sz),
          std::bind(&Session::response_sent, shared_from_this(),
            std::placeholders::_1, sz));
        resp->SerializeToArray(&snd_buf_[0], sz);
      }
    }

    void response_sent(boost::system::error_code ec, size_t len) {
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

    void clear_large_rcv_buffer() {
        decltype(large_rcv_buf_)().swap(large_rcv_buf_);
    }

    void clear_large_snd_buffer() {
        decltype(large_snd_buf_)().swap(large_snd_buf_);
    }


    local::stream_protocol::socket s_;
    //// IDs of this session
    uint64_t sid_;
    uint64_t pid_;
    uint64_t uid_;
    uint64_t gid_;
    /// buffer for the small object, need larger buffer for big
    //request.
    std::array<uint8_t, RECV_BUFSZ> rcv_buf_;
    std::vector<uint8_t> large_rcv_buf_;
    std::array<uint8_t, SEND_BUFSZ> snd_buf_;
    std::vector<uint8_t> large_snd_buf_;
    std::shared_ptr<LatteDispatcher> dispatcher_;
};
}
















