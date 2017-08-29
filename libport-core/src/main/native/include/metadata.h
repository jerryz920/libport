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


   Definition of metadata service client 
   Author: Yan Zhai
*/

#ifndef _LIBPORT_METADATA_H
#define _LIBPORT_METADATA_H


#include <string>
#include <vector>
#include "cpprest/http_client.h"
#include "pplx/pplx.h"

namespace latte {

class MetadataServiceDebugger;

class MetadataServiceClient {

  public:
    MetadataServiceClient() = delete;
    MetadataServiceClient(const MetadataServiceClient&) = delete;
    MetadataServiceClient& operator =(const MetadataServiceClient&) = delete;
    MetadataServiceClient(const std::string& server_url);


    /// the byte array is just 
    void post_new_principal(
        const std::string& principal_name,
        const std::string& principal_ip, int port_min,
        int port_max, const std::string& image_hash,
        const std::string& configs);

    void post_new_image(const std::string& image_hash,
        const std::string& source_url,
        const std::string& source_rev,
        const std::string& misc_conf);

//    void remove_principal(const std::string& host_ip, int port_min,
//        int port_max);
//
//    void remove_image(const std::string& image_hash);

    void post_object_acl(const std::string& obj_id,
        const std::string& requirements);

    //void endorse_principal(const std::string& host_ip, int port_min,
    //    int port_max, const std::string& endorsement);

    void endorse_image(const std::string& image_hash,
        const std::string& endorsement);

    //void endorse_source(const std::string& source_url,
    //    const std::string& source_revision, const std::string& endorsement);

    bool has_property(const std::string& principal_ip, int port,
        const std::string& property);
    bool can_access(const std::string& principal_ip, int port,
        const std::string& access_object);

  private:
    pplx::task<web::http::http_response> post_statement(const std::string& api_path,
        const std::string& target,
        const std::vector<std::string>& statements);

    web::http::client::http_client client_;

    /// just for test purpose
    friend class MetadataServiceDebugger;
};

}
#endif
