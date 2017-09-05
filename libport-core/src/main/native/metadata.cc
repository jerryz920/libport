
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
#include <iostream>


namespace {
  web::json::value format_request(const std::string& speaker,
        const std::vector<std::string>& statements) {
    web::json::value result = web::json::value::object(true);
    web::json::value otherValues = web::json::value::array();
    for (size_t i = 0; i < statements.size(); i++) { otherValues[i] = web::json::value::string(statements[i]);
    }
    result["principal"] = web::json::value::string(speaker);
    result["otherValues"] = otherValues;
    return result;
  }

  inline std::string format_netaddr(const std::string& ip, int port) {
    std::stringstream res;
    res << ip << ":" << port;
    return res.str();
  }

  inline std::string format_netaddr(const std::string& ip, int port_min, int port_max) {
    std::stringstream res;
    res << ip << ":" << port_min << "-" << port_max;
    return res.str();
  }

  inline std::string format_image_source(const std::string& source_url,
      const std::string& source_rev, const std::string& misc_conf) {
    std::stringstream image_source_info;
    image_source_info << source_url << "#" << source_rev << "#" << misc_conf;
    return image_source_info.str();
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
      try {
        auto v = res.get();
        consume_status(v, msg);
      } catch (std::runtime_error &e) {
        /// we don't need to do anything now
      }
    };
  }
}


namespace latte {
pplx::task<web::http::http_response> MetadataServiceClient::post_statement(
    const std::string& api_path, const std::string& speaker,
    const std::vector<std::string>& statements) {
  if (is_debug()) {
    log("");
    log("new metadata action: %s, target %s", api_path.c_str(), speaker.c_str());
    log("---start of statement---");
    for (auto s : statements) {
      log("%s;", s.c_str());
    }
    log("---end of statement ----");
  }

  web::http::http_request new_request(web::http::methods::POST);
  new_request.set_request_uri(api_path);
  new_request.headers().set_content_type("application/json");
  new_request.set_body(format_request(speaker, statements));
  return this->client_.request(new_request);
}

MetadataServiceClient::MetadataServiceClient(const std::string &server_url):
  client_(server_url) {
  }

void MetadataServiceClient::post_new_principal(const std::string& principal_name,
    const std::string& principal_ip, int port_min, int port_max,
    const std::string& image_hash, const std::string& configs) {

  this->post_statement("/postInstanceSet", "", {principal_name, image_hash,
      "image", format_netaddr(principal_ip, port_min, port_max), configs})
    .then(debug_task())
    .then(json_task("creating principal"))
    .then([this](pplx::task<web::json::value> v) -> pplx::task<web::http::http_response> {
        try {
          auto result = v.get();
          auto wrap = result["message"].as_string();
          auto start = wrap.find('\'');
          if (start == std::string::npos) {
            log_err("no quote found in message: %s", wrap.c_str());
            throw std::runtime_error("");
          }
          auto end = wrap.rfind('\'');
          auto key = wrap.substr(start + 1, end - start);
          return this->post_statement("/updateSubjectSet", "", {key});
        } catch(std::runtime_error &e) {
          return pplx::task_from_exception<web::http::http_response>(
            std::runtime_error(std::move(e)));
        }
    })
    .then(sink_task("updating subject")).wait();
}

void MetadataServiceClient::post_new_image(const std::string& image_hash,
        const std::string& source_url,
        const std::string& source_rev,
        const std::string& misc_conf) {
  this->post_statement("/postAttesterImage", "", 
      {image_hash})
    .then(debug_task())
    .then([&](web::http::http_response res) -> 
        pplx::task<web::http::http_response> {
        if (res.status_code() == web::http::status_codes::OK) {
          return this->post_statement("/postImageProperty", "", {
              image_hash, format_image_source(source_url, source_rev, misc_conf)
              });
        } else{
          return pplx::task_from_exception<web::http::http_response>(
            std::runtime_error(consume_status(res, "creating image")));
        }
        })
    .then(sink_task("posting image_src")).wait();
}

void MetadataServiceClient::post_object_acl(const std::string& object_id,
    const std::string& requirement) {
  this->post_statement("/postObjectAcl", "", {object_id, requirement})
    .then(debug_task())
    .then(sink_task("posting acl")).wait();
}

void MetadataServiceClient::endorse_image(const std::string& image_hash,
    const std::string& endorsement) {
  this->post_statement("/postImageProperty", "", {image_hash, endorsement})
    .then(debug_task())
    .then(sink_task("endorsing image")).wait();
}

bool MetadataServiceClient::has_property(const std::string& principal_ip,
    int port, const std::string& property) {
  std::cout << "this is executed!\n";
  return this->post_statement("/attestAppProperty", "", {format_netaddr(principal_ip, port),
      property})
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

bool MetadataServiceClient::can_access(const std::string& principal_ip, int port,
    const std::string& access_object) {
  return this->post_statement("/appAccessesObject", "", {format_netaddr(principal_ip, port),
      access_object})
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





}





