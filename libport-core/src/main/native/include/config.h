
#ifndef _LIBPORT_CONFIG_H
#define _LIBPORT_CONFIG_H

namespace latte {
/// to pass configure we now have a plain string, but we will leave
//space for the map.
const char *LEGACY_CONFIG_KEY = "LEGACY_CONFIG_STRING";

/// For object smaller than such size we will use pre-allocated buffer
// for larger one we dynamic allocate
const constexpr int RECV_BUFSZ = 8192;
const constexpr int SEND_BUFSZ = 8192;
const constexpr int PROTO_MAGIC = 0x1987;





}


#endif
