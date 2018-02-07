
#ifndef _LIBPORT_PROTO_TRAITS_H
#define _LIBPORT_PROTO_TRAITS_H

#include "proto/statement.pb.h"

//// bunch of traits to simplify the programming with templates
namespace latte {
namespace proto{
template<Command::Type type> struct statement_traits {
  typedef google::protobuf::Message msg_type;
  typedef google::protobuf::Message result_type;
}; 

#define DECL_STMT_TRAITS(TYPE, MSG_T, RES_T) \
template<> struct statement_traits<TYPE> { \
  typedef MSG_T msg_type; \
  typedef RES_T result_type; \
  static constexpr const char *name = #TYPE; \
  static constexpr const uint32_t value = static_cast<uint32_t>(TYPE); \
};


DECL_STMT_TRAITS(Command::CREATE_PRINCIPAL, Principal, Status);
DECL_STMT_TRAITS(Command::DELETE_PRINCIPAL, Principal, Status);
DECL_STMT_TRAITS(Command::GET_PRINCIPAL, Principal, Principal);
DECL_STMT_TRAITS(Command::GET_LOCAL_PRINCIPAL, Principal, Principal);
DECL_STMT_TRAITS(Command::ENDORSE_PRINCIPAL, EndorsePrincipal, Status);
DECL_STMT_TRAITS(Command::GET_METADATA_CONFIG, Empty, MetadataConfig);
/// Revoke is exact reverse of endorse, so just reuse it
DECL_STMT_TRAITS(Command::REVOKE_PRINCIPAL_ENDORSEMENT, EndorsePrincipal, Status);
DECL_STMT_TRAITS(Command::ENDORSE, Endorse, Status);
DECL_STMT_TRAITS(Command::REVOKE, Endorse, Status);
/// Some legacy command
DECL_STMT_TRAITS(Command::ENDORSE_MEMBERSHIP, EndorsePrincipal, Status);
DECL_STMT_TRAITS(Command::ENDORSE_ATTESTER_IMAGE, Endorse, Status);
DECL_STMT_TRAITS(Command::ENDORSE_BUILDER_IMAGE, Endorse, Status);
DECL_STMT_TRAITS(Command::ENDORSE_SOURCE_IMAGE, Endorse, Status);
DECL_STMT_TRAITS(Command::ENDORSE_IMAGE_PROPERTY, Endorse, Status);
DECL_STMT_TRAITS(Command::POST_ACL, PostACL, Status);
/// legacy checking command
DECL_STMT_TRAITS(Command::CHECK_PROPERTY, CheckProperty, Status);
DECL_STMT_TRAITS(Command::CHECK_ACCESS, CheckAccess, Status);
DECL_STMT_TRAITS(Command::CHECK_ATTESTATION, CheckAttestation, Attestation);
DECL_STMT_TRAITS(Command::CHECK_WORKER_ACCESS, CheckAccess, Status);
DECL_STMT_TRAITS(Command::CHECK_IMAGE_PROPERTY, CheckImage, Status);


/* Over complicated
template<typename V, typename V::Type value>
struct enum_traits {
  static constexpr const int v = 0;
  static constexpr const char *name = "invalid";

};

#define DECL_ENUM_TRAITS(Stmt, Value, Index) \
template<> struct enum_traits<Stmt, Value> { \
  static constexpr const int v = Index;      \
  static constexpr const char *name = #Value;      \
};
DECL_ENUM_TRAITS(Endorsement, Endorsement_Type_MEMBERSHIP, 0);
DECL_ENUM_TRAITS(Endorsement, Endorsement_Type_ATTESTER, 1);
DECL_ENUM_TRAITS(Endorsement, Endorsement_Type_BUILDER, 2);
DECL_ENUM_TRAITS(Endorsement, Endorsement_Type_SOURCE, 3);
DECL_ENUM_TRAITS(Endorsement, Endorsement_Type_OTHER, 4);

DECL_ENUM_TRAITS(Endorse, Endorse_Type_IMAGE, 0);
DECL_ENUM_TRAITS(Endorse, Endorse_Type_SOURCE, 1);

DECL_ENUM_TRAITS(Command, Command::CREATE_PRINCIPAL , 0);
DECL_ENUM_TRAITS(Command, Command::DELETE_PRINCIPAL , 1);
DECL_ENUM_TRAITS(Command, Command::ENDORSE_PRINCIPAL , 2);
DECL_ENUM_TRAITS(Command, Command::REVOKE_PRINCIPAL_ENDORSEMENT , 3);
DECL_ENUM_TRAITS(Command, Command::ENDORSE , 4);
DECL_ENUM_TRAITS(Command, Command::REVOKE , 5);
DECL_ENUM_TRAITS(Command, Command::GET_PRINCIPAL , 6);
DECL_ENUM_TRAITS(Command, Command::GET_LOCAL_PRINCIPAL , 20);
DECL_ENUM_TRAITS(Command, Command::GET_METADATA_CONFIG , 21);
DECL_ENUM_TRAITS(Command, Command::ENDORSE_MEMBERSHIP , 50);
DECL_ENUM_TRAITS(Command, Command::ENDORSE_ATTESTER_IMAGE , 51);
DECL_ENUM_TRAITS(Command, Command::ENDORSE_IMAGE_PROPERTY , 52);
DECL_ENUM_TRAITS(Command, Command::ENDORSE_SOURCE_IMAGE , 53);
DECL_ENUM_TRAITS(Command, Command::ENDORSE_BUILDER_IMAGE , 54);
DECL_ENUM_TRAITS(Command, Command::CHECK_PROPERTY , 100);
DECL_ENUM_TRAITS(Command, Command::CHECK_ATTESTATION , 101);
DECL_ENUM_TRAITS(Command, Command::CHECK_ACCESS , 102);
DECL_ENUM_TRAITS(Command, Command::POST_ACL , 103);
*/

}
}
#endif
