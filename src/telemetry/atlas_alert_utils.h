#ifndef __ATLAS_ALERT_UTILS_H__
#define __ATLAS_ALERT_UTILS_H__

#include <stdint.h>
#include "../utils/atlas_status.h"

/**
* @brief Parse telemetry push alert command
* @param[in] buf Raw command buffer
* @param[in] buf_len Raw command buffer length
* @param[out] push_rate Push interval
* @return status
*/
atlas_status_t atlas_alert_push_cmd_parse(const uint8_t *buf, uint16_t buf_len, uint16_t *push_rate);

/**
* @brief Parse telemetry push alert command
* @param[in] buf Raw command buffer
* @param[in] buf_len Raw command buffer length
* @param[out] scan_rate Scan rate interval
* @param[out] threshold Threshold value
* @return status
*/
atlas_status_t atlas_alert_threshold_cmd_parse(const uint8_t *buf, uint16_t buf_len, uint16_t *push_rate, char **threshold);

#endif /* __ATLAS_ALERT_UTILS_H__ */
