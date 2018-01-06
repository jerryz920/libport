
#include "proto/statement.pb.h"



namespace latte {

namespace comm {

using proto::Response;
using proto::Command;
//// All handlers have format of Process(Command) -> Response


Response create_principal(const Command &cmd) {
  return Response();
}

Response delete_principal(const Command &cmd) {
  return Response();
}

Response get_principal(const Command &cmd) {
  return Response();
}

Response endorse_principal(const Command &cmd) {
  return Response();
}

Response revoke_principal(const Command &cmd) {
  return Response();
}

Response endorse(const Command &cmd) {
  return Response();
}

Response revoke(const Command &cmd) {
  return Response();
}

Response get_local_principal(const Command &cmd) {
  return Response();
}

Response get_metadata_config(const Command &cmd) {
  return Response();
}

////////////////////Legacy APIs
Response post_acl(const Command &cmd) {
  return Response();
}

Response endorse_membership(const Command &cmd) {
  return Response();
}

Response endorse_attester(const Command &cmd) {
  return Response();
}

Response endorse_image(const Command &cmd) {
  return Response();
}

Response endorse_source(const Command &cmd) {
  return Response();
}

Response check_property(const Command &cmd) {
  return Response();
}

Response check_attestation(const Command &cmd) {
  return Response();
}

Response check_access(const Command &cmd) {
  return Response();
}

}


}


