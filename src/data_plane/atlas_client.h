#ifndef __ATLAS_CLIENT_H__
#define __ATLAS_CLIENT_H__

#include <stdint.h>

void atlas_init(char* user, char* client_id, uint16_t qos, uint16_t ppm, uint16_t pack_maxlen);

void atlas_pkt_received(int payload);

#endif /* __ATLAS_CLIENT_H__ */

