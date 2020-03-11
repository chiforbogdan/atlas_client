#ifndef __ATLAS_CLIENT_H__
#define __ATLAS_CLIENT_H__

#include <stdint.h>
#include "../utils/atlas_status.h"

void atlas_init(const char* user, const char* client_id, uint16_t qos, uint16_t ppm, uint16_t pack_maxlen);

void atlas_pkt_received(int payload);

atlas_status_t atlas_reputation_request(char *feature);

atlas_status_t send_feedback_command(char* payload);

#endif /* __ATLAS_CLIENT_H__ */

