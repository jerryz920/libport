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


   The export header for external usage of this client side port management library
   Author: Yan Zhai

*/


#ifndef _LIBPORT_H_
#define _LIBPORT_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif 

#define DEFAULT_DAEMON_PATH "/var/run/attguard.sock"
/// init:
//   * initialize the location of metadata service
//   * fill in the local port and exclude range
//   args:
//    * metadata_host: IP address or host name
//    * metadata_service: port address of metadata service


int create_image(const char *image_hash, const char *source_url,
    const char *source_rev, const char *misc_conf);
int post_object_acl(const char *obj_id, const char *requirement);
int attest_principal_property(const char *ip, uint32_t port, const char *prop);
int attest_principal_access(const char *ip, uint32_t port, const char *obj);

// log setting
void libport_set_log_level(int upto);


int libport_init(int run_as_iaas, const char *daemon_path);
int create_principal_new(uint64_t uuid, const char *image, const char *config,
    int nport, const char *new_ip);
int create_principal(uint64_t uuid, const char *image, const char *config,
    int nport);

int create_principal_with_allocated_ports(uint64_t uuid, const char *image,
    const char *config, const char * ip, int port_lo, int port_hi);

/// Legacy API
int create_image(const char *image_hash, const char *source_url,
    const char *source_rev, const char *misc_conf);

int post_object_acl(const char *obj_id, const char *requirement);

int endorse_image_new(const char *image_hash, const char *config,
    const char *endorsement);

/// Should delete this API
int endorse_image(const char *image_hash, const char *endorsement);

/// legacy API
int attest_principal_property(const char *ip, uint32_t port, const char *prop);

int attest_principal_access(const char *ip, uint32_t port, const char *obj);

int delete_principal(uint64_t uuid, uint64_t gn);

/// helper

char* get_principal(const char *ip, uint32_t lo, char **principal,
    size_t *size);

char* get_local_principal(uint64_t uuid, uint64_t gn, char **principal,
    size_t *size);

int get_metadata_config_easy(char *url, size_t *url_sz);

char* get_metadata_config(char **metadata_config, size_t *size);
int endorse_principal(const char *ip, uint32_t port, int type,
    const char *property);

int revoke_principal(const char *, uint32_t , int , const char *);

int endorse_image_latte(const char *id, const char *config, const char *property) ;
int endorse_source_latte(const char *url, const char *rev, const char *config,
    const char *property) ;

int revoke(const char *, const char *, int , const char *);

int endorse_membership(const char *ip, uint32_t port, const char *master);

int endorse_attester(const char *id, const char *config);

int endorse_builder(const char *id, const char *config);
int endorse_image_source(const char * id, const char * config, const char *url, const char *rev);


int check_property(const char *ip, uint32_t port, const char *property);

int check_access(const char *ip, uint32_t port, const char *object);

char* check_attestation(const char *ip, uint32_t port, char **attestation,
    size_t *size);



#ifdef __cplusplus
}
#endif




#endif
