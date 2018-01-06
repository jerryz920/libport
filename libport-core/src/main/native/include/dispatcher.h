
#ifndef _LIBPORT_DISPATCHER_H
#define _LIBPORT_DISPATCHER_H

#include "../proto/statement.pb.h"
#include <unordered_map>

namespace latte {


  template <typename Request, typename Response>
  class Dispatcher {
    public:
      typedef std::function<void(std::shared_ptr<Response>)> Writer;
      typedef std::function<std::shared_ptr<Response>(std::shared_ptr<Request>)> Handler;
      GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Dispatcher);
      virtual bool dispatch(std::shared_ptr<Request> cmd, Writer w) = 0;

  };






}


#endif
