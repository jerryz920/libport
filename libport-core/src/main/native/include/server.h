
#ifndef _LIBPORT_SERVER_H
#define _LIBPORT_SERVER_H


#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>
#include "utils.h"
#include "manager.h"
#include <pplx/threadpool.h>

namespace latte {
class Session;

/// Just an implementation.
class Server {
  public:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Server);
    Server(std::string ep):
      ep_(std::move(ep)),
      executor_(crossplat::threadpool::shared_instance()),
      service_(executor_.service()),
      listener_{service_, boost::asio::local::stream_protocol::endpoint(ep_)} {
      }

    void start();
  private:
    void new_session(std::shared_ptr<Session> session,
        const boost::system::error_code &ec);
    void start_accept();
    std::string ep_;
    crossplat::threadpool &executor_;
    boost::asio::io_service &service_;
    boost::asio::local::stream_protocol::acceptor listener_;
    std::shared_ptr<LatteDispatcher> manager_;
};


}


#endif
