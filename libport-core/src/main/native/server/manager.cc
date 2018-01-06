

#include "proto/config.h"
#include "proto/statement.pb.h"
#include "proto/utils.h"
#include "dispatcher.h"


namespace latte {


class DispatcherImpl: public Dispatcher<proto::Command, proto::Response> {

  public:

    bool dispatch(std::shared_ptr<proto::Command> cmd, Writer w) {



      return true;
    }





};


}

