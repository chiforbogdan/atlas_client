#include "atlas_coap_client.h"

atlas_status_t
atlas_coap_client_request(const char *uri, int port, const uint8_t *req_payload, size_t req_payload_len, atlas_coap_client_cb_t cb)
{
    if (!uri)
        return ATLAS_COAP_INVALID_URI;
    if (!cb)
        return ATLAS_INVALID_CALLBACK;

    return ATLAS_OK;
}
