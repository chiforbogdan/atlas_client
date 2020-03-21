#ifndef __ATLAS_CLIENT_H__
#define __ATLAS_CLIENT_H__

#include <stdint.h>
#include<time.h>
#include "../utils/atlas_status.h"

#define ATLAS_IDENTITY_MAX_LEN (64)

typedef enum
{
    ATLAS_SENSOR_TEMPERATURE = 0,

} atlas_sensor_t;

typedef struct atlas_feedback
{
    /* Device identity */
    char identity[ATLAS_IDENTITY_MAX_LEN];
    /* Sensor type */
    atlas_sensor_t sensor_type;
    /* Sensor score */
    uint16_t sensor_score;
    /* Response time score */
    uint16_t time_score;
} atlas_feedback_t;

/**
* @brief Init atlas framework
* @param[in] qos Maximum firewall QoS value
* @param[in] ppm Maximum firewall packet-per-minute value
* @param[in] packet_maxlen Maximum firewall packet length value
* @param[out] identity ATLAS identity which must be used as MQTT client id
* @return status
*/
atlas_status_t atlas_init(uint16_t qos, uint16_t ppm, uint16_t packet_maxlen, char **identity);

/**
* @brief Packet receive callback. Increments the internal statistics which will
* synchronized with atlas client
* @param[in] payload_len Recieved packet payload length
* @return none
*/
void atlas_pkt_received(int payload_len);

/**
* @brief Execute a reputation request for a feature (category)
* @param[in] sensor_type Sensor type
* @param[out] identity The most trusted identity within a category
* @return status
*/
atlas_status_t atlas_reputation_request(atlas_sensor_t sensor_type, char **identity);

/**
* @brief Send reputation feedback
* @param[in] feedback Array of feedback scores
* @param[in] feedback_len Number of feedback elements
* @return status
*/
atlas_status_t atlas_reputation_feedback(atlas_feedback_t *feedback, size_t feedback_len);

#endif /* __ATLAS_CLIENT_H__ */

