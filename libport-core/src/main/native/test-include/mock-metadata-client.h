
#pragma once

#include "metadata.h"
#include <string>
#include "jutils/mock.h"

namespace latte { 

class MockMetadataClient: public MetadataServiceClient {
  public:
    MOCK_METHOD6(std::string, post_new_principal,
        const std::string&, principal_name,
        const std::string&, principal_ip, int, port_min,
        int, port_max, const std::string&, image_hash,
        const std::string&, configs, override)

    MOCK_VOID_METHOD6(remove_principal,
        const std::string&, principal_name,
        const std::string&, principal_ip, int, port_min,
        int, port_max, const std::string&, image_hash,
        const std::string&, configs, override)

    MOCK_VOID_METHOD4(post_new_image,
        const std::string&, image_hash,
        const std::string&, source_url,
        const std::string&, source_rev,
        const std::string&, misc_conf, override)

    MOCK_VOID_METHOD2(post_object_acl,
        const std::string&, obj_id,
        const std::string&, requirements, override)

    MOCK_VOID_METHOD3(endorse_image,
        const std::string&, image_hash,
        const std::string&, endorsement,
        const std::string&, config, override)

    MOCK_METHOD4(bool, has_property,
        const std::string&, principal_ip, uint32_t, port,
        const std::string&, property,
        const std::string&, bearer, override)

    MOCK_METHOD4(bool, can_access,
        const std::string&, principal_ip, uint32_t, port,
        const std::string&, access_object, 
        const std::string&, bearer, override)

    MOCK_VOID_METHOD2(endorse_attester,
        const std::string&, image_id,
        const std::string&, config, override)

    MOCK_VOID_METHOD2(endorse_builder,
        const std::string&, image_id,
        const std::string&, config, override)

    MOCK_VOID_METHOD4(endorse_membership,
        const std::string&, ip,
        uint32_t, port,
        const std::string&, endorse,
        const std::string&, config, override)

    MOCK_VOID_METHOD2(endorse_attester_on_source,
        const std::string&, source_id,
        const std::string&, config, override)

    MOCK_VOID_METHOD2(endorse_builder_on_source,
        const std::string&, source_id,
        const std::string&, config, override)

    MOCK_VOID_METHOD3(endorse_source,
        const std::string&, image_id,
        const std::string&, config,
        const std::string&, source, override)

    MOCK_METHOD3(std::string, attest, const std::string&, ip,
        uint32_t, port, const std::string&, bearer, override)

    MOCK_METHOD4(bool, can_worker_access, const std::string&, ip,
        uint32_t, port, const std::string&, object,
        const std::string&, bearer, override)

};
}