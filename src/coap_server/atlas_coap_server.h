#ifndef __ATLAS_COAP_SERVER_H__
#define __ATLAS_COAP_SERVER_H__

#include "../utils/atlas_status.h"
#include "../scheduler/atlas_scheduler.h"

/**
* @brief Start CoAP server
* @param[in] hostname Server hostname
* @param[in] port Server port
* @return status
*/
atlas_status_t atlas_coap_server_start(const char *hostname, const char *port);

#endif /* __ATLAS_COAP_SERVER_H__ */
