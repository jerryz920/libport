
/*
   The FreeBSD Copyright

   Copyright 1992-2017 The FreeBSD Project. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.  Redistributions in binary
   form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials
   provided with the distribution.  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND
   CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   The views and conclusions contained in the software and documentation are
   those of the authors and should not be interpreted as representing official
   policies, either expressed or implied, of the FreeBSD Project.


   Implementation of metadata service clients
   Author: Yan Zhai
*/


#include "log.h"
#include "metadata.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "utils.h"
#include "safe.h"
#include <iostream>
#include <stdio.h>


namespace {
  web::json::value format_request(const std::string& speaker,
        const std::vector<std::string>& statements, const std::string& bearer) {
    web::json::value result = web::json::value::object(true);
    web::json::value otherValues = web::json::value::array();
    for (size_t i = 0; i < statements.size(); i++) { otherValues[i] = web::json::value::string(statements[i]);
    }
    result["principal"] = web::json::value::string(speaker);
    result["otherValues"] = otherValues;
    if (bearer.size() > 0) {
      result["bearerRef"] = web::json::value::string(bearer);
    }
    return result;
  }



  inline std::string consume_status(const web::http::http_response& res, const std::string& msg) {
        if (res.status_code() != web::http::status_codes::OK) {
          std::stringstream err;
          err << "error in " << msg << ":" << res.status_code() << "\n";
          latte::log(err.str().c_str());
          return err.str();
        }
        return "";
  }

  typedef std::function<pplx::task<web::json::value>(web::http::http_response res)> JsonTask;

  JsonTask json_task(const char *msg) {
    return [msg](web::http::http_response res) {
        if (res.status_code() == web::http::status_codes::OK) {
          return res.extract_json();
        } else {
          return pplx::task_from_exception<web::json::value>(
              std::runtime_error(consume_status(res, msg)));
        }
    };
  }

  typedef std::function<pplx::task<web::http::http_response>(web::http::http_response res)> DebugTask;
  DebugTask debug_task() {
    return [](web::http::http_response res) {
      if (latte::is_debug()) {
        latte::log("response body: \n%s", res.to_string().c_str());
      }
      return pplx::task_from_result(res);
    };
  }

  typedef std::function<void (pplx::task<web::http::http_response> res)> SinkTask;
  SinkTask sink_task(const char *msg) {
    return [msg](pplx::task<web::http::http_response> res) {
        auto v = res.get();
        consume_status(v, msg);
    };
  }


  std::string extract_safe_identity(web::json::value result) {
    auto wrap = result["message"].as_string();
    auto start = wrap.find('\'');
    if (start == std::string::npos) {
      std::stringstream ss;
      ss <<"no quote found in message: " << wrap.c_str();
      throw std::runtime_error(ss.str());
    }
    auto end = wrap.rfind('\'');
    if (end == start) {
      std::stringstream ss;
      ss << "ill formed return message, can not find key: " << wrap.c_str();
      throw std::runtime_error(ss.str());
    }
    return wrap.substr(start + 1, end - start - 1);
  }
}


namespace latte {

static std::once_flag trace_once;
static FILE* ftrace = NULL;
pplx::task<web::http::http_response> MetadataServiceClient::post_statement(
    const std::string& api_path, const std::string& speaker,
    const std::vector<std::string>& statements, const std::string& bearer_ref) {
  if (is_debug()) {
    log("");
    log("new metadata action: %s, target %s, ref: %s", api_path.c_str(), speaker.c_str(),
        bearer_ref.c_str());
    log("---start of statement---");
    for (auto s : statements) {
      log("%s;", s.c_str());
    }
    log("---end of statement ----");
  }

  web::http::http_request new_request(web::http::methods::POST);
  new_request.set_request_uri(api_path);
  new_request.headers().set_content_type("application/json");
  new_request.set_body(format_request(speaker, statements, bearer_ref));
  if (is_debug()) {
    static int counter = 0;
    std::call_once(trace_once, []() {
          pid_t pid = getpid();
          ftrace = std::fopen(("/tmp/libport-trace" + utils::itoa(pid)).c_str(), "w");
        });

    if (ftrace) {
      auto full_uri = client_->base_uri().to_string();
      if (*full_uri.rbegin() == '/') {
        full_uri.pop_back();
      } 
      if (api_path[0] == '/') {
        full_uri += api_path;
      } else {
        full_uri.push_back('/');
        full_uri += api_path;
      }
      fprintf(ftrace, "#%d \ncurl %s -XPOST -d '%s'\n\n", counter++, full_uri.c_str(),
          format_request(speaker, statements, bearer_ref).serialize().c_str());
      fflush(ftrace);
    }
  }
  return this->client_->request(new_request);
}

MetadataServiceClient::MetadataServiceClient(const std::string &server_url,
    const std::string& myid):
  client_(utils::make_unique<web::http::client::http_client>(server_url)),
  myid_(myid) {
  }

std::string MetadataServiceClient::post_new_principal(const std::string &speaker,
    const std::string& principal_name,
    const std::string& principal_ip, int port_min, int port_max,
    const std::string& image_hash, const std::string& configs) {

  std::string identity;
  this->post_statement("/postInstanceSet", speaker, {principal_name, image_hash,
      "image", utils::format_netaddr(principal_ip, port_min, port_max), configs})
    .then(debug_task())
    .then(json_task("creating principal"))
    .then([&](pplx::task<web::json::value> v) -> pplx::task<web::http::http_response> {
        try {
          auto key = extract_safe_identity(v.get());
          identity = key;
          return this->post_statement("/updateSubjectSet", utils::format_netaddr(principal_ip, port_min,
                port_max), {key});
        } catch(std::runtime_error &e) {
          return pplx::task_from_exception<web::http::http_response>(
            std::runtime_error(std::move(e)));
        }
    })
    .then(sink_task("updating subject")).wait();
    return identity;
}


void MetadataServiceClient::post_new_image(const std::string &speaker,
    const std::string& image_hash,
        const std::string& source_url,
        const std::string& source_rev,
        const std::string& misc_conf) {
  /* FIXME: Using IAAS_IDENTITY should be a BUG but we will fix them later */
  this->post_statement("/postAttesterImage", speaker, 
      {image_hash, misc_conf})
    .then(debug_task())
    .then([&](web::http::http_response res) -> 
        pplx::task<web::http::http_response> {
        if (res.status_code() == web::http::status_codes::OK) {
          return this->post_statement("/postImageProperty", IAAS_IDENTITY, {
              image_hash, utils::format_image_source(source_url, source_rev), misc_conf
              });
        } else{
          return pplx::task_from_exception<web::http::http_response>(
            std::runtime_error(consume_status(res, "creating image")));
        }
        })
    .then(sink_task("posting image_src")).wait();
}

void MetadataServiceClient::post_object_acl(const std::string &speaker,
    const std::string& object_id,
    const std::string& requirement) {
  // FIXME: this is a problem with SAFE. we temporarily use alice for object
  // acl setting...
  std::string actual_id = object_id;
  if (object_id.find(":") == std::string::npos) {
    actual_id = speaker + ":" + object_id;
  }
  this->post_statement("/postObjectAcl", speaker, {actual_id, requirement})
    .then(debug_task())
    .then(sink_task("posting acl")).wait();
}

void MetadataServiceClient::endorse_image(const std::string &speaker,
    const std::string& image_hash,
    const std::string& endorsement, const std::string& config) {
  this->post_statement("/postImageProperty", speaker, {image_hash, config, endorsement})
    .then(debug_task())
    .then(sink_task("endorsing image")).wait();
}

void MetadataServiceClient::endorse_membership(const std::string &speaker,
    const std::string &ip,
    uint32_t port, const std::string &property, const std::string &config) {
  std::string identity;
  this->post_statement("/postWorkerSet", speaker, {property,
      utils::format_netaddr(ip, port), config})
    .then(debug_task())
    /* This part is workarounded in the proxy
    .then(json_task("endorsing membership"))
    .then([&](pplx::task<web::json::value> v) -> pplx::task<web::http::http_response> {
        try {
          auto key = extract_safe_identity(v.get());
          identity = key;
          return this->post_statement("/updateSubjectSet", utils::format_netaddr(ip, port), {key});
        } catch(std::runtime_error &e) {
          return pplx::task_from_exception<web::http::http_response>(
            std::runtime_error(std::move(e)));
        }})
    */
    .then(sink_task("updating membership")).wait();
}

void MetadataServiceClient::endorse_membership(const std::string &speaker,
    const std::string &ip,
    uint32_t port, uint64_t gn, const std::string &property, const std::string &config) {
  std::string identity;
  this->post_statement("/postWorkerSet", speaker, {property,
      utils::format_netaddr(ip, port),
      //utils::itoa(gn),
      config})
    .then(debug_task())
    /*
    .then(json_task("endorsing membership"))
    .then([&](pplx::task<web::json::value> v) -> pplx::task<web::http::http_response> {
        try {
          auto key = extract_safe_identity(v.get());
          identity = key;
          return this->post_statement("/updateSubjectSet", utils::format_netaddr(ip, port), {key});
        } catch(std::runtime_error &e) {
          return pplx::task_from_exception<web::http::http_response>(
            std::runtime_error(std::move(e)));
        }})
    */
    .then(sink_task("updating membership")).wait();
}

void MetadataServiceClient::endorse_builder_on_source(const std::string &speaker,
    const std::string &source_id,
    const std::string &config) {
  this->post_statement("/postSourceImage", speaker, 
      {source_id, config})
    .then(debug_task())
    .then(sink_task("endorsing builder")).wait();
}

void MetadataServiceClient::endorse_builder(const std::string &speaker,
    const std::string &image_id,
    const std::string &config) {
  this->post_statement("/postBuilderImage", IAAS_IDENTITY, 
      {image_id, config})
    .then(debug_task())
    .then(sink_task("endorsing builder")).wait();
}
void MetadataServiceClient::endorse_attester_on_source(const std::string &speaker,
    const std::string &source_id,
    const std::string &config) {
  this->post_statement("/postAttesterSource", IAAS_IDENTITY, 
      {source_id, config})
    .then(debug_task())
    .then(sink_task("endorsing attester")).wait();
}

void MetadataServiceClient::endorse_attester(const std::string &speaker,
    const std::string &image_id,
    const std::string &config) {
  this->post_statement("/postAttesterImage", IAAS_IDENTITY, 
      {image_id, config})
    .then(debug_task())
    .then(sink_task("endorsing attester")).wait();
}

void MetadataServiceClient::endorse_source(const std::string &speaker,
    const std::string &image_id,
    const std::string &config, const std::string &source) {
  this->post_statement("/postImageSource", IAAS_IDENTITY, 
      {image_id, config, source})
    .then(debug_task())
    .then(sink_task("endorsing source")).wait();
}

bool MetadataServiceClient::has_property(const std::string &,
    const std::string& principal_ip,
    uint32_t port, const std::string& property, const std::string& bearer) {
  return this->post_statement("/attestAppProperty", ATTEST_IDENTITY, {
      utils::format_netaddr(principal_ip, port), property}, bearer)
    .then(debug_task())
    .then(json_task("attest property"))
    .then([](pplx::task<web::json::value> v) -> bool {
      try {
        auto msg = v.get()["message"].as_string();
        if (msg.find("programHasProperty") == std::string::npos) {
          log("no property found: %s", msg.c_str());
          return false;
        }
        return true;
      } catch (std::runtime_error &e) {
        /// we alreay caught it, no need to do again
        return false;
      }
    }).get();
}

bool MetadataServiceClient::can_access(const std::string &,
    const std::string& principal_ip, uint32_t port,
    const std::string& access_object, const std::string& bearer) {
  return this->post_statement("/appAccessesObject", ATTEST_IDENTITY, {
      utils::format_netaddr(principal_ip, port), access_object}, bearer)
    .then(debug_task())
    .then(json_task("attest access"))
    .then([](pplx::task<web::json::value> v) -> bool {
      try {
        auto msg = v.get()["message"].as_string();
        if (msg.find("approveAccess") == std::string::npos) {
          log("not approving access: %s", msg.c_str());
          return false;
        }
        return true;
      } catch (std::runtime_error &e) {
        /// we alreay caught it, no need to do again
        return false;
      }
    }).get();
}

bool MetadataServiceClient::can_worker_access(const std::string &,
    const std::string& principal_ip, uint32_t port,
    const std::string& access_object, const std::string& bearer) {
  return this->post_statement("/workerAccessesObject", ATTEST_IDENTITY, {
      utils::format_netaddr(principal_ip, port), access_object}, bearer)
    .then(debug_task())
    .then(json_task("attest worker access"))
    .then([](pplx::task<web::json::value> v) -> bool {
      try {
        auto msg = v.get()["message"].as_string();
        if (msg.find("approveAccess") == std::string::npos) {
          log("not approving access: %s", msg.c_str());
          return false;
        }
        return true;
      } catch (std::runtime_error &e) {
        /// we alreay caught it, no need to do again
        return false;
      }
    }).get();
}



std::string MetadataServiceClient::attest(const std::string &speaker,
    const std::string &ip,
    uint32_t port, const std::string &bearer) {
  return this->post_statement("/attestInstance", speaker, {
      utils::format_netaddr(ip, port)}, bearer)
    .then(debug_task())
    .then(json_task("attest instance"))
    .then([](pplx::task<web::json::value> v) -> std::string {
      try {
        return v.get()["message"].as_string();
      } catch (std::runtime_error &e) {
        /// we alreay caught it, no need to do again
        return "";
      }
    }).get();
}

void MetadataServiceClient::remove_principal(
    const std::string &speaker,
        const std::string& principal_name,
        const std::string& principal_ip, int port_min,
        int port_max, const std::string& image_hash,
        const std::string& configs)
{
  this->post_statement("/retractInstanceSet", speaker, {principal_name, image_hash,
      "image", utils::format_netaddr(principal_ip, port_min, port_max), configs})
    .then(debug_task())
    .then(json_task("deleting principal")).wait();
}


bool MetadataServiceClient::image_has_property(
    const std::string &speaker,
    const std::string &image,
    const std::string &config,
    const std::string &property) {
  return this->post_statement("/checkImgProperty", ATTEST_IDENTITY, {
      image, config, property})
    .then(debug_task())
    .then(json_task("check image property"))
    .then([](pplx::task<web::json::value> v) -> bool {
      try {
        auto msg = v.get()["message"].as_string();
        if (msg.find("hasSafetyProperty") == std::string::npos) {
          log("image not having desired property: %s", msg.c_str());
          return false;
        }
        return true;
      } catch (std::runtime_error &e) {
        /// we alreay caught it, no need to do again
        return false;
      }
    }).get();
}


}





