
#ifndef _LIBPORT_SESSION_H
#define _LIBPORT_SESSION_H

#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "proto/statement.pb.h"
#include "config.h"
#include "log.h"
#include "utils.h"
#include "comm.h"
#include "manager.h"

using namespace boost::asio;
using google::protobuf::io::CodedInputStream;


namespace latte {
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
    Session(io_service &service, std::shared_ptr<LatteDispatcher> dispatcher);

    local::stream_protocol::socket &socket() { return s_; }

  private:

    bool authenticate();
    void proto_start();
    void header_received(const boost::system::error_code &ec, size_t len);
    void command_received(const boost::system::error_code &ec, size_t len, uint8_t *data);
    void write_response(std::shared_ptr<proto::Response> resp);
    void response_sent(const boost::system::error_code &ec, size_t len);

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



#endif
