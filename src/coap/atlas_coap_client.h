#ifndef __ATLAS_COAP_CLIENT_H__
#define __ATLAS_COAP_CLIENT_H__

#include <stdint.h>
#include <stdlib.h>
#include "atlas_coap_response.h"
#include "atlas_coap_method.h"
#include "../utils/atlas_status.h"

#define ATLAS_COAP_CLIENT_DEFAULT_TIMEOUT_MS (5000)

typedef void (*atlas_coap_client_cb_t)(const char *uri_path, atlas_coap_response_t resp_status, const uint8_t *resp_payload,
size_t resp_payload_len);

/**
 * @brief Set DTLS PSK info to be used in subsequent requests
 * @param[in] identity DTLS identity
 * @param[in] psk DTLS Pre-shared key
 * @return status
 */
atlas_status_t atlas_coap_client_set_dtls_info(const char *identity, const char *psk);

/**
 * @brief Execute a CoAP request
 * @param[in] uri CoAP URI (e.g. coap://<IP>:<PORT>/<PATH>?<QUERY>)
 * @param[in] method CoAP method
 * @param[in] req_payload CoAP request payload
 * @param[in] req_payload_len CoAP request payload length
 * @param[in] timeout Timeout value (ms)
 * @param[in] cb Response callback
 * @return status
 */
atlas_status_t atlas_coap_client_request(const char *uri, atlas_coap_method_t method,
const uint8_t *req_payload, size_t req_payload_len, uint16_t timeout, atlas_coap_client_cb_t cb);

#endif /* __ATLAS_COAP_CLIENT_H__ */
