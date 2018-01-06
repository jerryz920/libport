
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

   Protobuf utility library
   Author: Yan Zhai

*/
#ifndef _LIBPORT_PROTO_UTILS_H
#define _LIBPORT_PROTO_UTILS_H


#include "statement.pb.h"

namespace latte {
namespace proto{

  class ResponseWrapper {

    public:

      GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ResponseWrapper);

      ResponseWrapper() = delete;
      ResponseWrapper(std::unique_ptr<Response> r):
        r_(std::move(r)) {
      }
      ResponseWrapper(Response *r): r_(std::unique_ptr<Response>(r)) {}
      ResponseWrapper(ResponseWrapper &&other){
        r_.swap(other.r_);
      }

      static Response* make_status_response(bool success, const std::string &msg) {
        Response* resp = Response::default_instance().New();
        resp->set_type(Response::Type::Response_Type_STATUS);
        Status status;
        status.set_success(success);
        status.set_info(msg);
        auto result = resp->mutable_result();
        result->PackFrom(status);
        return resp;
      }

      inline int status_int() const {
        Status state;
        if (!r_->result().UnpackTo(&state)) {
          return -1;
        }
        return (int)state.success();
      }

      inline std::pair<int, std::string> status() const {
        Status state;
        if (!r_->result().UnpackTo(&state)) {
          return std::make_pair(-1, "");
        }
        return std::make_pair((int)state.success(),
            std::move(*state.mutable_info()));
      }

      inline std::unique_ptr<Principal> extract_principal() const {
        Principal* p = Principal::default_instance().New();
        if (!r_->result().UnpackTo(p)) {
          return nullptr;
        }
        return std::unique_ptr<Principal>(p);
      }

      inline std::unique_ptr<MetadataConfig> extract_metadata_config() const {
        MetadataConfig *m = MetadataConfig::default_instance().New();
        if (!r_->result().UnpackTo(m)) {
          return nullptr;
        }
        return std::unique_ptr<MetadataConfig>(m);
      }

      inline std::unique_ptr<Attestation> extract_attetation() const {
        Attestation *a = Attestation::default_instance().New();
        if (!r_->result().UnpackTo(a)) {
          return nullptr;
        }
        return std::unique_ptr<Attestation>(a);
      }

    private:
      std::unique_ptr<Response> r_;


  };


  template<typename V, typename I>
    static inline void set_type(V& v, I type) {
      v.set_type(static_cast<typename V::Type>(type));
    }

  template<typename V, typename I>
    static inline void set_type(V* v, I type) {
      v->set_type(static_cast<typename V::Type>(type));
    }

  template<typename Msg>
    static char* wrap_proto_msg(const Msg &p, char **buf, size_t *size) {
      *size = p.ByteSize();
      *buf = (char*) malloc(*size);
      if (!*buf) {
        return nullptr;
      }

      if (!p.SerializeToArray(*buf, *size)) {
        free(*buf);
        *buf = nullptr;
      }
      return *buf;
    }

}
}





#endif
