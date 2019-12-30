#ifndef __ATLAS_COAP_CLIENT_H__
#define __ATLAS_COAP_CLIENT_H__

#include <stdint.h>
#include <stdlib.h>
#include "../utils/atlas_status.h"

typedef void (*atlas_coap_client_cb_t)(int status, const uint8_t *resp_payload,
size_t resp_payload_len);

/**
 * @brief Execute a CoAP request
 * @param[in] uri CoAP URI
 * @param[in] cb Response callback
 * @return status
 */
atlas_status_t atlas_coap_client_request(const char *uri, int port, const uint8_t *req_payload, size_t req_payload_len, atlas_coap_client_cb_t cb);

#endif /* __ATLAS_COAP_CLIENT_H__ */
