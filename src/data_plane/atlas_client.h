#ifndef __ATLAS_CLIENT_H__
#define __ATLAS_CLIENT_H__

#include <stdint.h>
#include<time.h>
#include "../utils/atlas_status.h"

typedef struct feedback_struct {
    char* clientID;
    char* feature;
    uint16_t feature_value;
    uint16_t reponse_time;
    struct feedback_struct *next;
} feedback_struct_t;


void atlas_init(const char* user, const char* client_id, uint16_t qos, uint16_t ppm, uint16_t pack_maxlen);

void atlas_pkt_received(int payload);

/**
* @brief Execute a reputation request for a feature (category)
* @param[in] feature Reputation feature
* @param[out] clientid The most trusted clientid within a category. The buffer must be allocated by the caller.
* @param[in] clientid_len The length of the clientid buffer. This function will copy at most clientid_len - 1 bytes
*                         into the clientid buffer
* @return status
*/
atlas_status_t atlas_reputation_request(const char *feature, char *clientid, size_t clientid_len);

void init_feedback_command(feedback_struct_t *feedback_entry);

#endif /* __ATLAS_CLIENT_H__ */

