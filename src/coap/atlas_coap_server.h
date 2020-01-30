#ifndef __ATLAS_COAP_SERVER_H__
#define __ATLAS_COAP_SERVER_H__

#include <stdint.h>
#include "../utils/atlas_status.h"
#include "../scheduler/atlas_scheduler.h"
#include "atlas_coap_method.h"
#include "atlas_coap_response.h"

typedef enum _atlas_coap_server_mode
{
    /* UDP transport mode */
    ATLAS_COAP_SERVER_MODE_UDP = (1 << 0),

    /* DTLS with PSK transport mode */
    ATLAS_COAP_SERVER_MODE_DTLS_PSK = (1 << 1),

    /* UDP + DTLS PSK transport modes */
    ATLAS_COAP_SERVER_MODE_BOTH = (ATLAS_COAP_SERVER_MODE_UDP | ATLAS_COAP_SERVER_MODE_DTLS_PSK),
} atlas_coap_server_mode_t;

typedef atlas_coap_response_t (*atlas_coap_server_cb_t)(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len,
                                                        uint8_t **resp_payload, size_t *resp_payload_len);


/**
* @brief Start CoAP server
* @param[in] port Server port
* @param[in] server_mode Server mode (UDP, DTLS PSK, both)
* @param[in] psk Pre-shared key (for DTLS)
* @return status
*/
atlas_status_t atlas_coap_server_start(uint16_t port, atlas_coap_server_mode_t server_mode, const char *psk);

/**
 * @brief Add CoAP resource
 * @param[in] uri_path Resource URI path
 * @param[in] method CoAP method
 * @param[in] cb Callback
 * @return status
 */
atlas_status_t atlas_coap_server_add_resource(const char *uri_path, atlas_coap_method_t method, atlas_coap_server_cb_t cb);

/**
 * @brief Delete CoAP resource
 * @param[in] uri_path Resource URI path
 * @param[in] method CoAP method
 * @return none
 */
void atlas_coap_server_del_resource(const char *uri_path, atlas_coap_method_t method);

#endif /* __ATLAS_COAP_SERVER_H__ */
