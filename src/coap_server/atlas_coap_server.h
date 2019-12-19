#ifndef __ATLAS_COAP_SERVER_H__
#define __ATLAS_COAP_SERVER_H__

#include "../utils/atlas_status.h"

/**
* @brief Start CoAP server
* @param[in] hostname Server hostname
* @param[in] port Server port
* @return status
*/
atlas_status_t atlas_coap_server_start(const char *hostname, const char *port);

/**
* @brief Run CoAP server main loop
* @return none
*/
void atlas_coap_server_loop();

#endif /* __ATLAS_COAP_SERVER_H__ */
