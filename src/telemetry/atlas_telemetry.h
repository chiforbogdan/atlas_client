#ifndef __ATLAS_TELEMETRY_H__
#define __ATLAS_TELEMETRY_H__

#include <stdint.h>

#define ATLAS_TELEMETRY_URI_LEN        (64)
#define ATLAS_TELEMETRY_USE_THRESHOLD  (1)
#define ATLAS_TELEMETRY_SKIP_THRESHOLD (0)

typedef void (*atlas_telemetry_payload_cb)(uint8_t **, uint16_t*, uint8_t); 

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

/**
 * @brief Set external push rate for a telemetry feature
 * @param[in] uri Telemetry feature URI
 * @param[in] ext_push External push interval in seconds. If this value if 0, then
 * the telemetry feature will be pushed right away to the gateway
 */
void atlas_telemetry_ext_push_set(const char *uri, uint16_t ext_push);

/**
 * @brief Set internal scan rate for a telemetry feature
 * @param[in] uri Telemetry feature URI
 * @param[in] int_scan Internal scan interval in seconds
 */
void atlas_telemetry_int_scan_set(const char *uri, uint16_t ext_push);

#endif /* __ATLAS_TELEMETRY_H__ */
