#ifndef __ATLAS_ALERT_UTILS_H__
#define __ATLAS_ALERT_UTILS_H__

#include <stdint.h>
#include "../utils/atlas_status.h"

/**
* @brief Parse telemetry alert command
* @param[in] buf Raw command buffer
* @param[in] buf_len Raw command buffer length
* @param[out] ext_push External push interval
* @param[out] int_scan Internal scan interval
* @param[out] threshold Threshold value
* @return status
*/
atlas_status_t atlas_alert_cmd_parse(const uint8_t *buf, uint16_t buf_len,
                                     uint16_t **ext_push, uint16_t **int_scan, char **threshold);

#endif /* __ATLAS_ALERT_UTILS_H__ */
