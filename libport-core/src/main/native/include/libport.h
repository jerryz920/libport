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

#ifdef __cplusplus
extern "C" {
#endif 

/// init:
//   * initialize the location of metadata service
//   * fill in the local port and exclude range
//   args:
//    * metadata_host: IP address or host name
//    * metadata_service: port address of metadata service
int libport_init(const char *metadata_host, const char *metadata_service,
    int port_per_principal);


/// create_principal:
//   * create a new principal, set up port range
//  args:
//   * uuid: the ID of given principal
//   * vargs: string arrays, each representing a statement, which
//      will be appended into "otherValues" field
int create_principal(uint64_t uuid, ...);


/// speak_for:
//   * speak for a principal in the name of current principal
//   * the principal spoken to must be itself or direct child
//  args:
//   * uuid: the ID to speak for
//   * vargs: similar like "create_principal", sending a list of
//     statements inside "otherValues" field
int speak_for(uint64_t uuid, ...);

/// delete_principal:
//   * remove a principal, and withdraw the mapping, as well as
//   * the statement
int delete_principal(uint64_t uuid);


#ifdef __cplusplus
}
#endif




#endif
