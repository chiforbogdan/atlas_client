#ifndef __ATLAS_CONFIG_H__
#define __ATLAS_CONFIG_H__

#include "atlas_status.h"

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
* @brief Get gateway hostname
* @return Gateway hostname
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

#endif /* __ATLAS_CONFIG_H__ */
