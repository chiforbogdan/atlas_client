#include <string.h>
#include <stdlib.h>
#include "atlas_config.h"

#define ATLAS_MAX_PORT_VAL (65535)

/* Gateway hostname */
static char *gw_hostname;

/* Gateway port */
static char *gw_port;

/* Client local interface */
static char *local_iface;

atlas_status_t
atlas_cfg_set_hostname(const char *hostname)
{
    if (!hostname)
        return ATLAS_INVALID_INPUT;

    gw_hostname = malloc(strlen(hostname));
    strcpy(gw_hostname, hostname);

    return ATLAS_OK;
}

const char*
atlas_cfg_get_hostname()
{
    return gw_hostname;
}

atlas_status_t
atlas_cfg_set_port(const char *port)
{
    int port_val;

    if (!port)
        return ATLAS_INVALID_INPUT;

    port_val = atoi(port);
    if (port_val <= 0 || port_val > ATLAS_MAX_PORT_VAL)
        return ATLAS_INVALID_INPUT;

    gw_port = malloc(strlen(port));
    strcpy(gw_port, port);

    return ATLAS_OK;
}

const char*
atlas_cfg_get_port()
{
    return gw_port;
}

atlas_status_t
atlas_cfg_set_local_iface(const char *iface)
{
    if (!iface)
        return ATLAS_INVALID_INPUT;

    local_iface = malloc(strlen(iface));
    strcpy(local_iface, iface);

    return ATLAS_OK;
}

const char*
atlas_cfg_get_local_iface()
{
    return local_iface;
}

