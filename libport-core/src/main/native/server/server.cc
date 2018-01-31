


#include "server.h"
#include "session.h"
#include "manager.h"


namespace latte {

void Server::start() {
  manager_ = get_manager();
  start_accept();
  service_.run();
}

void Server::start_accept() {
  auto session = Session::create(service_, manager_);
  log("waiting for new session");
  listener_.async_accept(session->socket(),
      std::bind(&Server::new_session, this, session, std::placeholders::_1));
}

void Server::new_session(std::shared_ptr<Session> session,
    const boost::system::error_code &ec) {
  if (!ec) {
    log("starting new session");
    session->start();
  }
  start_accept();
}

}
