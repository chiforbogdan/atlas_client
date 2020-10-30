#ifndef __ATLAS_COMMAND_EXECUTE_UTILS_H__
#define __ATLAS_COMMAND_EXECUTE_UTILS_H__

#include <stdint.h>
#include "../utils/atlas_status.h"


/**
* @brief Parse command execution payload
* @param[in] buf Raw command buffer
* @param[in] buf_len Raw command buffer length
* @return status
 */
atlas_status_t atlas_alert_command_execution_parse(const uint8_t *buf, uint16_t buf_len);

#endif /* __ATLAS_COMMAND_EXECUTE_UTILS_H__ */ 