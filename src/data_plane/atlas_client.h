#ifndef __ATLAS_CLIENT_H__
#define __ATLAS_CLIENT_H__

#include <stdint.h>
#include<time.h>
#include "../utils/atlas_status.h"
#include "atlas_data_plane.h"

void atlas_init(const char* user, const char* client_id, uint16_t qos, uint16_t ppm, uint16_t pack_maxlen);

void atlas_pkt_received(int payload);

atlas_status_t atlas_reputation_request(char *feature);

void init_feedback_command(feedback_struct_t *feedback_entry);

#endif /* __ATLAS_CLIENT_H__ */

