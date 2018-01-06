

#include <unordered_map>
#include "proto/config.h"
#include "proto/statement.pb.h"
#include "proto/utils.h"
#include "proto/traits.h"
#include "manager.h"
#include "pplx/threadpool.h"
#include "pplx/pplxtasks.h"


namespace latte {

using proto::Response;
using proto::Command;

class LatteAttestationManager: public LatteDispatcher {

  public:
    LatteAttestationManager(const std::string &metadata_server):
      LatteAttestationManager(metadata_server,crossplat::threadpool::shared_instance() ) {
    }
    LatteAttestationManager(const std::string &metadata_server,
      crossplat::threadpool &pool): executor_(pool) {
    }

    bool dispatch(std::shared_ptr<proto::Command> cmd, Writer w) override {
      auto handler = find_handler(cmd->type());
      std::shared_ptr<Response> result;
      if (handler) {

      } else {
        std::string typedesc = proto::Command::Type_Name(cmd->type());
        result = std::make_shared<proto::Response>(
            proto::ResponseWrapper::make_status_response(false,
              "not handler found for type " + typedesc));
      }
      w(result);
      return true;
    }

    //// In fact we should separate these things out as a standalone Manager class,
    // while the dispatcher accepts registeration of handlers. So the separation
    // is cleaner. But I have no time for that kind of shit.
  private:

    typedef std::shared_ptr<Response> (LatteAttestationManager::*RawHandler)
      (std::shared_ptr<Command>);

    void register_handler(proto::Command::Type type, RawHandler h) {
      dispatch_table_[static_cast<uint32_t>(type)] = std::bind(h,
          this, std::placeholders::_1);
    }

    Handler find_handler(proto::Command::Type type) const {
      auto res = dispatch_table_.find(static_cast<uint32_t>(type));
      if (res == dispatch_table_.end()) {
        return nullptr;
      }
      return res->second;
    }


    void init() {
      register_handler(proto::Command::CREATE_PRINCIPAL, 
          &LatteAttestationManager::create_principal);
      register_handler(proto::Command::DELETE_PRINCIPAL, 
          &LatteAttestationManager::delete_principal);
      register_handler(proto::Command::GET_PRINCIPAL, 
          &LatteAttestationManager::get_principal);
      register_handler(proto::Command::ENDORSE_PRINCIPAL, 
          &LatteAttestationManager::endorse_principal);
      register_handler(proto::Command::REVOKE_PRINCIPAL_ENDORSEMENT, 
          &LatteAttestationManager::revoke_principal);
      register_handler(proto::Command::ENDORSE, 
          &LatteAttestationManager::endorse);
      register_handler(proto::Command::REVOKE, 
          &LatteAttestationManager::revoke);
      register_handler(proto::Command::GET_LOCAL_PRINCIPAL, 
          &LatteAttestationManager::get_local_principal);
      register_handler(proto::Command::GET_METADATA_CONFIG, 
          &LatteAttestationManager::get_metadata_config);
      register_handler(proto::Command::POST_ACL, 
          &LatteAttestationManager::post_acl);
      register_handler(proto::Command::ENDORSE_MEMBERSHIP, 
          &LatteAttestationManager::endorse_membership);
      register_handler(proto::Command::ENDORSE_ATTESTER_IMAGE, 
          &LatteAttestationManager::endorse_attester);
      register_handler(proto::Command::ENDORSE_BUILDER_IMAGE, 
          &LatteAttestationManager::endorse_builder);
      register_handler(proto::Command::ENDORSE_SOURCE_IMAGE, 
          &LatteAttestationManager::endorse_source);
      register_handler(proto::Command::CHECK_PROPERTY, 
          &LatteAttestationManager::check_property);
      register_handler(proto::Command::CHECK_ATTESTATION, 
          &LatteAttestationManager::check_attestation);
      register_handler(proto::Command::CHECK_ACCESS, 
          &LatteAttestationManager::check_access);
    }


    std::shared_ptr<Response> create_principal(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> delete_principal(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> get_principal(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> endorse_principal(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> revoke_principal(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> endorse(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> revoke(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> get_local_principal(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> get_metadata_config(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    ////////////////////Legacy APIs
    std::shared_ptr<Response> post_acl(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> endorse_membership(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> endorse_attester(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> endorse_builder(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> endorse_source(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> check_property(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> check_attestation(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::shared_ptr<Response> check_access(std::shared_ptr<Command> cmd) {
      return std::shared_ptr<Response>();
    }

    std::unordered_map<uint32_t, Handler> dispatch_table_;
    crossplat::threadpool & executor_;


};

static std::shared_ptr<LatteAttestationManager> instance;

void init_manager(const std::string &metadata_url) {
  instance = std::make_shared<LatteAttestationManager>(metadata_url);
}

void init_manager(const std::string &metadata_url, crossplat::threadpool &pool) {
  instance = std::make_shared<LatteAttestationManager>(metadata_url, pool);
}

std::shared_ptr<LatteDispatcher> get_manager() {
  if (instance) {
    return instance;
  }
  /// Must init manager first to call it
  throw std::runtime_error("must initialize the latte manager first");
}



}

