
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

   simple protobuf communication protocol
   Author: Yan Zhai

*/
#ifndef _LIBPORT_PROTO_COMM_H
#define _LIBPORT_PROTO_COMM_H

#include "google/protobuf/message.h"
#include "google/protobuf/io/coded_stream.h"
#include "utils.h"
#include "proto/traits.h"

namespace latte {

  using google::protobuf::Message;
  using google::protobuf::io::CodedOutputStream;
  using google::protobuf::io::CodedInputStream;
    

  static constexpr int COMM_HEADER_SZ = 8;

  /// Should move these to common library.

  static inline int proto_send_msg(int sock, const Message &cmd) {
      utils::Buffer buffer(sizeof(uint32_t) * 2 + cmd.ByteSize());
      CodedOutputStream::WriteLittleEndian32ToArray(cmd.ByteSize(),
          buffer.buf());
      CodedOutputStream::WriteLittleEndian32ToArray(0x1987,
          buffer.buf() + sizeof(uint32_t));
      cmd.SerializeToArray(buffer.buf() + sizeof(uint32_t) * 2, cmd.ByteSize());
      return utils::reliable_send(sock, buffer.buf(), buffer.size());
    }

    template<class ProtoMessage>
    static int proto_recv_msg(int sock, ProtoMessage *result) {

      unsigned char buffer[COMM_HEADER_SZ];
      int ret = utils::reliable_recv(sock, buffer, COMM_HEADER_SZ);
      if (ret != 0) {
        return ret;
      }
      uint32_t size, magic;
      google::protobuf::io::CodedInputStream::ReadLittleEndian32FromArray(buffer, &size);
      google::protobuf::io::CodedInputStream::ReadLittleEndian32FromArray(buffer, &magic);

      // +1 for defense
      utils::Buffer response_buf(size + 1);
      ret = utils::reliable_recv(sock, response_buf.buf(), size);
      if (ret != 0) {
        return ret;
      }
      if (result->ParseFromArray(response_buf.buf(), size)) {
        return utils::PARSE_ERROR;
      }
      return 0;
    }

template<proto::Command::Type type>
proto::Command prepare(
    const typename proto::statement_traits<type>::msg_type &statement) {
  proto::Command cmd;
  cmd.set_id(utils::gen_rand_uint64());
  cmd.set_type(type);
  auto stmt_field = cmd.mutable_statement();
  stmt_field->PackFrom(statement);
  return cmd;
}




}



#endif
