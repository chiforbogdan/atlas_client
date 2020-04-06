#ifndef __ATLAS_TELEMETRY_H__
#define __ATLAS_TELEMETRY_H__

#include <stdint.h>

#define ATLAS_TELEMETRY_USE_THRESHOLD  (1)
#define ATLAS_TELEMETRY_SKIP_THRESHOLD (0)

typedef void (*atlas_telemetry_payload_cb)(uint8_t **, uint16_t*, uint8_t); 

/**
* @brief Add a telemetry feature
* @param[in] path Telemetry feature URI path on the gateway side (must be unique)
* @return none
*/
void atlas_telemetry_add(const char *uri_path, atlas_telemetry_payload_cb payload_cb); 

/**
* @brief Delete a telemetry feature
* @param[in] uri_path Telemetry feature URI path
* @return none
*/
void atlas_telemetry_del(const char *uri_path);

/**
* @brief Push all telemetry features to gateway
* @return none
*/
void atlas_telemetry_push_all();

/**
 * @brief Set push rate for a telemetry feature
 * @param[in] uri_path Telemetry feature URI path
 * @param[in] push_rate Push interval in seconds. If this value if 0, then
 * the telemetry feature will be pushed right away to the gateway
 */
void atlas_telemetry_push_set(const char *uri_path, uint16_t push_rate);

/**
 * @brief Set internal scan rate for a telemetry feature
 * @param[in] uri_path Telemetry feature URI path
 * @param[in] scan_rate Internal scan interval in seconds
 */
void atlas_telemetry_threshold_set(const char *uri_path, uint16_t scan_rate);

#endif /* __ATLAS_TELEMETRY_H__ */
