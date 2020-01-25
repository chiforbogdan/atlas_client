#ifndef __ATLAS_TELEMETRY_H__
#define __ATLAS_TELEMETRY_H__

#include <stdint.h>

#define ATLAS_TELEMETRY_URI_LEN (64)

typedef void (*atlas_telemetry_payload_cb)(uint8_t **, uint16_t*); 

/**
* @brief Add a telemetry feature
* @param[in] uri Telemetry feature URI on the gateway side (must be unique)
* @return none
*/
void atlas_telemetry_add(const char *uri, atlas_telemetry_payload_cb payload_cb); 

/**
* @brief Delete a telemetry feature
* @param[in] uri Telemetry feature URI
* @return none
*/
void atlas_telemetry_del(const char *uri);

/**
* @brief Push all telemetry features to gateway
* @return none
*/
void atlas_telemetry_push_all();

#endif /* __ATLAS_TELEMETRY_H__ */
