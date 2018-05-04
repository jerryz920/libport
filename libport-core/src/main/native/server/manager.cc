

#include <unordered_map>
#include "proto/config.h"
#include "proto/statement.pb.h"
#include "proto/utils.h"
#include "proto/traits.h"
#include "manager.h"
#include "pplx/threadpool.h"
#include "pplx/pplxtasks.h"
#include "metadata.h"
#include "utils.h"
#include <jutils/config/simple.h>
#include "config.h"


namespace latte {

using proto::Response;
using proto::Command;

class LatteAttestationManager: public LatteDispatcher {

  public:
    typedef std::unordered_multimap<uint64_t, std::shared_ptr<proto::Principal>> PrincipalMap;

    LatteAttestationManager(const std::string &metadata_server):
      LatteAttestationManager(metadata_server,crossplat::threadpool::shared_instance() ) {
    }
    LatteAttestationManager(const std::string &metadata_server,
      crossplat::threadpool &pool): executor_(pool),
      metadata_service_(utils::make_unique<MetadataServiceClient>(
            metadata_server, latte::config::myid())) {
          init();
    }
    LatteAttestationManager(MetadataServiceClient* service):
      LatteAttestationManager(service,crossplat::threadpool::shared_instance()) {
    }

    LatteAttestationManager(MetadataServiceClient* service,
        crossplat::threadpool &pool): executor_(pool),
        metadata_service_(service) {
          init();
    }

    bool dispatch(std::shared_ptr<proto::Command> cmd, Writer w) override {
      auto handler = find_handler(cmd->type());
      std::shared_ptr<Response> result;
      if (handler) {
        try {
          //// TODO: validate the speaker field if needed
          result = handler(cmd);
        } catch (const std::runtime_error &e) {
          std::string errmsg = "runtime error " + std::string(e.what());
          log_err(errmsg.c_str());
          result = proto::make_shared_status_response(false, errmsg);
        } catch (const web::json::json_exception &e) {
          std::string errmsg = "json error " + std::string(e.what());
          log_err(errmsg.c_str());
          result = proto::make_shared_status_response(false, errmsg);
        } catch(const web::http::http_exception &e) {
          std::string errmsg = "http error " + std::string(e.what());
          log_err(errmsg.c_str());
          result = proto::make_shared_status_response(false, errmsg);
        }
      } else {
        std::string typedesc = proto::Command::Type_Name(cmd->type());
        result = proto::make_shared_status_response(false,
              "not handler found for type " + typedesc);
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


    void init(uint64_t init_gn = 0) {
      gn_ = init_gn;
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
      register_handler(proto::Command::CHECK_WORKER_ACCESS, 
          &LatteAttestationManager::check_worker_access);
      register_handler(proto::Command::CHECK_IMAGE_PROPERTY, 
          &LatteAttestationManager::check_image_property);
      register_handler(proto::Command::FREE_CALL,
          &LatteAttestationManager::free_call);
      register_handler(proto::Command::GUARD_CALL,
          &LatteAttestationManager::guard_call);
      register_handler(proto::Command::LINK_IMAGE,
          &LatteAttestationManager::link_image);
    }


    static inline std::string principal_name(const proto::Principal &p) {
      std::stringstream ss;
      ss << p.id() << ":" << p.gn();
      return ss.str();
    }


    std::shared_ptr<Response> free_call(std::shared_ptr<Command> cmd) {
      auto freecall = proto::CommandWrapper::extract_free_call(*cmd);
      auto proto_values = freecall->othervalues();
      std::vector<std::string> other_values(proto_values.begin(), proto_values.end());
      auto res = metadata_service_->free_call(cmd->auth(),
          freecall->cmd(), other_values);
      return proto::make_shared_status_response(res, "");
    }
    std::shared_ptr<Response> guard_call(std::shared_ptr<Command> cmd) {
      auto guardcall = proto::CommandWrapper::extract_guard_call(*cmd);
      auto proto_values = guardcall->othervalues();
      std::vector<std::string> other_values(proto_values.begin(), proto_values.end());
      auto res = metadata_service_->guard_call(cmd->auth(),
          guardcall->cmd(), other_values);
      return proto::make_shared_status_response(res, "");
    }

    std::shared_ptr<Response> link_image(std::shared_ptr<Command> cmd) {
      auto linkimage = proto::CommandWrapper::extract_link_image(*cmd);
      auto host = linkimage->host();
      if (host.size() == 0) {
        host = cmd->auth();
      }
      auto &image = linkimage->image();
      auto res = metadata_service_->link_image(cmd->auth(), host, image);
      return proto::make_shared_status_response(res, "");
    }


    std::shared_ptr<Response> create_principal(std::shared_ptr<Command> cmd) {
      auto p = proto::CommandWrapper::extract_principal(*cmd);
      if (!p) {
        return proto::make_shared_status_response(false,
              "principal not found or mal-formed");
      }
      p->set_gn(gn());
      auto image_store = p->code().image_store();
      if (image_store.size() == 0) {
        image_store = cmd->auth();
      }
      //auto res = metadata_service_->post_new_principal(cmd->auth(), principal_name(*p),
      //    ip, p->auth().port_lo(), p->auth().port_hi(),
      //    p->code().image(), p->code().config().at(LEGACY_CONFIG_KEY));
      auto &confmap = p->code().config();
      /// copy the stuff to make the interface clean from any protobuf dependency
      std::unordered_map<std::string, std::string> configs(confmap.begin(), confmap.end());
      metadata_service_ -> create_instance(cmd->auth(), principal_name(*p),
          p->code().image(), p->auth().ip(), p->auth().port_lo(), p->auth().port_hi(),
          image_store, configs);

      p->set_speaker(cmd->pid());
      add_principal(p->id(), p);
      return proto::make_shared_status_response(true, "");
    }


    std::shared_ptr<Response> delete_principal(std::shared_ptr<Command> cmd) {
      auto p = proto::CommandWrapper::extract_principal(*cmd);
      if (!p) {
        return proto::make_shared_status_response(false,
              "principal not found or mal-formed");
      }

      uint64_t maxgn = 0;
      std::shared_ptr<proto::Principal> latest = nullptr;
      plock_.lock();
      auto matched = principals_.equal_range(p->id());
      for (auto i = matched.first; i != matched.second; ++i) {
        if (maxgn <= i->second->gn()) {
          maxgn = i->second->gn();
          latest = i->second;
        }
      }
      if (latest) {
        auto speaker = latest->speaker();
        if ((speaker != cmd->pid() && cmd->uid() != 0)) {
          plock_.unlock();
          return proto::make_shared_status_response(false,
              "privilege not matching");
        }
      }
      principals_.erase(matched.first, matched.second);
      //// authenticate if deletion is allowed!

      /// deleting latest principal
      if (latest) {
        auto speaker = latest->speaker();
        auto name = principal_name(*latest);
        auto ip = latest->auth().ip();
        //auto plo = latest->auth().port_lo();
        //auto phi = latest->auth().port_hi();
        auto image = latest->code().image();
        auto config = latest->code().config().at(LEGACY_CONFIG_KEY);
        log("deleting principal: id = %u, maxgn = %u, latest = %p, speaker = %u, pid = %u, uid = %u",
            p->id(), maxgn, latest.get(), speaker, cmd->pid(), cmd->uid());

        plock_.unlock();
        metadata_service_->delete_instance(cmd->auth(), name);
        //metadata_service_->remove_principal(cmd->auth(),name, ip, plo, phi, image, config);
        return proto::make_shared_status_response(true, "");
      } else {
        plock_.unlock();
        log("deleting %u, %u, latest principal not found", p->id(), p->gn());
        return proto::make_shared_status_response(false,
              "latest principal not found");
      }

    }

    std::shared_ptr<Response> get_principal(std::shared_ptr<Command> ) {
      /// not implemented yet
      return proto::not_implemented();
    }

    std::shared_ptr<Response> endorse_principal(std::shared_ptr<Command> ) {
      /// not implemented yet
      return proto::not_implemented();
    }

    std::shared_ptr<Response> revoke_principal(std::shared_ptr<Command> ) {
      /// not implemented yet
      return proto::not_implemented();
    }

    std::shared_ptr<Response> endorse(std::shared_ptr<Command> cmd) {
      auto endorse = proto::CommandWrapper::extract_endorse(*cmd);
      if (endorse->endorsements_size() == 0) {
        return proto::make_shared_status_response(false,
            "must provide at least one property");
      }
      auto &image = endorse->id();
      auto &property = endorse->endorsements(0).property();
      auto &value = endorse->endorsements(0).value();
      //auto &config = endorse->config().at(LEGACY_CONFIG_KEY);
      metadata_service_->endorse(cmd->auth(), image, property, value);
      return proto::make_shared_status_response(true, "");
    }

    std::shared_ptr<Response> revoke(std::shared_ptr<Command> ) {
      return proto::not_implemented();
    }

    std::shared_ptr<Response> get_local_principal(std::shared_ptr<Command> cmd) {
      auto p = proto::CommandWrapper::extract_principal(*cmd);
      if (!p) {
        return proto::make_shared_status_response(false,
              "principal not found or mal-formed");
      }

      uint64_t maxgn = 0;
      std::shared_ptr<proto::Principal> latest = nullptr;
      std::lock_guard<std::mutex> guard(plock_);
      auto matched = principals_.equal_range(p->id());
      for (auto i = matched.first; i != matched.second; ++i) {
        if (maxgn <= i->second->gn()) {
          maxgn = i->second->gn();
          latest = i->second;
        }
      }
      if (latest) {
        return proto::make_shared_principal_response(*latest);
      } else {
        return proto::make_shared_status_response(false, "not found");
      }
    }

    std::shared_ptr<Response> get_metadata_config(std::shared_ptr<Command>) {
      proto::MetadataConfig config;
      config.set_ip(config::metadata_service_ip());
      config.set_port(config::metadata_service_port());
      return proto::make_shared_metadata_config_response(config);
    }

    ////////////////////Legacy APIs
    std::shared_ptr<Response> post_acl(std::shared_ptr<Command> cmd) {
      auto acl = proto::CommandWrapper::extract_post_acl(*cmd);
      /// we only use the first name
      if (acl->policies_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one policy");
      }
      metadata_service_->post_object_acl(cmd->auth(), acl->name(), acl->policies(0));
      return proto::make_shared_status_response(true, "");
    }

    std::shared_ptr<Response> endorse_membership(std::shared_ptr<Command> cmd) {
      auto endorse_p = proto::CommandWrapper::extract_endorse_principal(*cmd);
      if (endorse_p->endorsements_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one endorsement");
      }
      auto gn = endorse_p->principal().gn();
      auto &target_ip = endorse_p->principal().auth().ip();
      auto target_port = endorse_p->principal().auth().port_lo();
      auto &target_config = endorse_p->principal().code().config().at(LEGACY_CONFIG_KEY);
      auto &statement = endorse_p->endorsements(0);
      metadata_service_->endorse_membership(cmd->auth(), target_ip, target_port, gn, statement.property(),
          target_config);
      return proto::make_shared_status_response(true, "");
    }

    std::shared_ptr<Response> endorse_attester(std::shared_ptr<Command> cmd) {
      auto endorse = proto::CommandWrapper::extract_endorse(*cmd);
      auto id = endorse->id();
      auto &config = endorse->config().at(LEGACY_CONFIG_KEY);
      if (endorse->type() == proto::Endorse::SOURCE) {
        /// metadata_service_->endorse_source(id, statement);
        metadata_service_->endorse_attester_on_source(cmd->auth(),id, config);
      } else {
        //// Need add a type
        metadata_service_->endorse_attester(cmd->auth(),id, config);
      }
      return proto::make_shared_status_response(true, "");
    }

    std::shared_ptr<Response> endorse_builder(std::shared_ptr<Command> cmd) {
      auto endorse = proto::CommandWrapper::extract_endorse(*cmd);
      auto id = endorse->id();
      if (endorse->endorsements_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one endorsement");
      }
      auto &config = endorse->config().at(LEGACY_CONFIG_KEY);
      if (endorse->type() == proto::Endorse::SOURCE) {
        /// metadata_service_->endorse_source(id, statement);
        metadata_service_->endorse_builder_on_source(cmd->auth(),id, config);
      } else {
        //// Need add a type
        metadata_service_->endorse_builder(cmd->auth(),id, config);
      }
      return proto::make_shared_status_response(true, "");
    }

    std::shared_ptr<Response> endorse_source(std::shared_ptr<Command> cmd) {
      auto endorse = proto::CommandWrapper::extract_endorse(*cmd);
      auto id = endorse->id();
      if (endorse->endorsements_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one endorsement");
      }
      auto &statement = endorse->endorsements(0);
      auto &config = endorse->config().at(LEGACY_CONFIG_KEY);
      if (endorse->type() == proto::Endorse::SOURCE) {
        /// metadata_service_->endorse_source(id, statement);
        return proto::make_shared_status_response(false, "source endorse this is image only");
      } else {
        //// Need add a type
        metadata_service_->endorse_image(cmd->auth(), id, statement.property(), config);
      }
      return proto::make_shared_status_response(true, "");
    }

    std::shared_ptr<Response> check_property(std::shared_ptr<Command> cmd) {
      auto check = proto::CommandWrapper::extract_check_property(*cmd);
      auto &ip = check->principal().auth().ip();
      auto port = check->principal().auth().port_lo();
      if (check->properties_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one property");
      }
      auto &prop = check->properties(0);
      return proto::make_shared_status_response(
          metadata_service_->has_property(cmd->auth(), ip, port, prop, ""), "");
    }

    std::shared_ptr<Response> check_attestation(std::shared_ptr<Command> cmd) {
      auto attest = proto::CommandWrapper::extract_check_attestation(*cmd);
      auto &ip = attest->principal().auth().ip();
      auto port = attest->principal().auth().port_lo();
      proto::Attestation a;
      a.set_content(metadata_service_->attest(cmd->auth(), ip, port, ""));
      return proto::make_shared_attestation_response(a);
    }

    std::shared_ptr<Response> check_access(std::shared_ptr<Command> cmd) {
      auto check = proto::CommandWrapper::extract_check_access(*cmd);
      auto &ip = check->principal().auth().ip();
      auto port = check->principal().auth().port_lo();
      if (check->objects_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one object");
      }
      auto &object = check->objects(0);
      return proto::make_shared_status_response(
          metadata_service_->can_access(cmd->auth(),ip, port, object, ""), "");
    }

    std::shared_ptr<Response> check_worker_access(std::shared_ptr<Command> cmd) {
      auto check = proto::CommandWrapper::extract_check_access(*cmd);
      auto &ip = check->principal().auth().ip();
      auto port = check->principal().auth().port_lo();
      if (check->objects_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one object");
      }
      auto &object = check->objects(0);
      return proto::make_shared_status_response(
          metadata_service_->can_worker_access(cmd->auth(),ip, port, object, ""), "");
    }

    std::shared_ptr<Response> check_image_property(std::shared_ptr<Command> cmd) {
      auto check = proto::CommandWrapper::extract_check_image(*cmd);
      auto &image = check->image();
      auto &confmap = check->config();
      if (check->property_size() == 0) {
        return proto::make_shared_status_response(false, "must provide one property");
      }
      auto &config = confmap.at(LEGACY_CONFIG_KEY);
      auto &property = check->property(0);
      return proto::make_shared_status_response(
          metadata_service_->image_has_property(cmd->auth(), image, config,
            property), "");
    }

    void add_principal(uint64_t id, std::shared_ptr<proto::Principal> p) {
      std::lock_guard<std::mutex> guard(plock_);
      principals_.insert(std::make_pair(id, std::move(p)));
    }

    uint64_t gn() {
      return gn_++;
    }

    std::unordered_map<uint32_t, Handler> dispatch_table_;
    crossplat::threadpool & executor_;
    std::unique_ptr<MetadataServiceClient> metadata_service_;

    std::mutex plock_;
    PrincipalMap principals_;
    uint64_t gn_;
    /// maybe we could use image but not now I think.

};

static std::shared_ptr<LatteAttestationManager> instance;

void init_manager(const std::string &metadata_url) {
  instance = std::make_shared<LatteAttestationManager>(metadata_url);
}

void init_manager(const std::string &metadata_url, crossplat::threadpool &pool) {
  instance = std::make_shared<LatteAttestationManager>(metadata_url, pool);
}

void init_manager(MetadataServiceClient *client) {
  instance = std::make_shared<LatteAttestationManager>(client);
}


std::shared_ptr<LatteDispatcher> get_manager() {
  if (instance) {
    return instance;
  }
  /// Must init manager first to call it
  throw std::runtime_error("must initialize the latte manager first");
}



}

