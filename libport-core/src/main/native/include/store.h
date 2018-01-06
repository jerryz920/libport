

#ifndef _LIBPORT_STORE_H
#define _LIBPORT_STORE_H

#include "google/protobuf/message.h"
#include "utils.h"

class StoreManager {
  public:
    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(StoreManager);
    StoreManager() {}
    virtual bool add(const google::protobuf::Message &msg) = 0;
    virtual bool remove(const google::protobuf::Message &msg) = 0;
    virtual std::vector<std::shared_ptr<google::protobuf::Message>> load() = 0;
};


#endif
