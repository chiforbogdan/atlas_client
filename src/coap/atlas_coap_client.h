#ifndef __ATLAS_COAP_CLIENT_H__
#define __ATLAS_COAP_CLIENT_H__

#include <stdint.h>
#include <stdlib.h>
#include "atlas_coap_response.h"
#include "../utils/atlas_status.h"

typedef void (*atlas_coap_client_cb_t)(atlas_coap_response_t resp_status, const uint8_t *resp_payload,
size_t resp_payload_len);

/**
 * @brief Execute a CoAP request
 * @param[in] uri CoAP URI
 * @param[in] port CoAP server port
 * @param[in] req_payload CoAP request payload
 * @param[in] req_payload_len CoAP request payload length
 * @param[in] cb Response callback
 * @return status
 */
atlas_status_t atlas_coap_client_request(const char *uri, uint16_t port,
const uint8_t *req_payload, size_t req_payload_len, atlas_coap_client_cb_t cb);

#endif /* __ATLAS_COAP_CLIENT_H__ */
