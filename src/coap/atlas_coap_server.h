#ifndef __ATLAS_COAP_SERVER_H__
#define __ATLAS_COAP_SERVER_H__

#include <stdint.h>
#include "../utils/atlas_status.h"
#include "../scheduler/atlas_scheduler.h"
#include "atlas_coap_method.h"

typedef void (*atlas_coap_server_cb_t)(const char *uri_path, uint8_t **payload, uint16_t *payload_len);

/**
* @brief Start CoAP server
* @param[in] hostname Server hostname
* @param[in] port Server port
* @return status
*/
atlas_status_t atlas_coap_server_start(const char *hostname, const char *port);

/**
 * @brief Add CoAP resource
 * @param[in] uri_path Resource URI path
 * @param[in] method CoAP method
 * @return status
 */
atlas_status_t atlas_coap_server_add_resource(const char *uri_path, atlas_coap_method_t method, atlas_coap_server_cb_t cb);

#endif /* __ATLAS_COAP_SERVER_H__ */
