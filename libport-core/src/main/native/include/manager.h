
#ifndef _LIBPORT_MANAGER_H
#define _LIBPORT_MANAGER_H

#include "dispatcher.h"
#include "proto/statement.pb.h"

namespace latte {

  typedef Dispatcher<proto::Command, proto::Response> LatteDispatcher;
  void init_manager(const std::string &metadata_url);
  std::shared_ptr<LatteDispatcher> get_manager();

}


#endif
