#ifndef __ATLAS_CONFIG_H__
#define __ATLAS_CONFIG_H__

#include "atlas_status.h"

#define ATLAS_PORT_MAX_LEN         (6)
#define ATLAS_IP_MAX_LEN           (16)
#define ATLAS_URI_MAX_LEN          (256)
#define ATLAS_URI_HOSTNAME_MAX_LEN (128)

/**
* @brief Set gateway hostname
* @param[in] hostname Gateway hostname
* @return none
*/
atlas_status_t atlas_cfg_set_hostname(const char *hostname);

/**
* @brief Get gateway hostname
* @return Gateway hostname
*/
const char* atlas_cfg_get_hostname();

/**
* @brief Set gateway port
* @param[in] port Gateway port
* @return none
*/
atlas_status_t atlas_cfg_set_port(const char *port);

/**
* @brief Get gateway port
* @return Gateway port
*/
const char* atlas_cfg_get_port();

/**
* @brief Set local interface
* @param[in] iface Local interface name
* @return none
*/
atlas_status_t atlas_cfg_set_local_iface(const char *iface);

/**
* @brief Get local interface
* @return Local interface name
*/
const char* atlas_cfg_get_local_iface();

/**
 * @brief Get IP address for the configured local interface
 * @param[out] ip IP address
 * @return none
 */
atlas_status_t atlas_cfg_get_local_ip(char *ip);

/**
* @brief Set local port
* @param[in] port Local port
* @return none
*/
atlas_status_t atlas_cfg_set_local_port(const char *port);

/**
* @brief Get local port
* @return Local port
*/
uint16_t atlas_cfg_get_local_port();

/**
* @brief Get gateway URI from given path (prepend schema://<gateway_hostname>:<gateway_port>/
* @param[in] path CoAP path
* @param[out] uri CoAP URI
* return none
*/
void atlas_cfg_coap_get_uri(const char *path, char *uri);

/**
 * @brief Get local IP address concatenated with port for the configured local interface
 * @param[out] ip IP address+port
 * @return none
 */
atlas_status_t atlas_cfg_get_local_ipPort(char *ipPort);

#endif /* __ATLAS_CONFIG_H__ */
