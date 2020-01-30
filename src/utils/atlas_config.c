#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
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

atlas_status_t
atlas_cfg_get_local_ip(char *ip)
{
    int fd;
    struct ifreq ifr;
    
    if (!ip)
        return ATLAS_INVALID_INPUT;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return ATLAS_GENERAL_ERR;

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, local_iface, IFNAMSIZ - 1);
    
    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    sprintf(ip, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    return ATLAS_OK;
}
