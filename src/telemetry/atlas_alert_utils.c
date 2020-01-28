#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "atlas_alert_utils.h"
#include "../commands/atlas_command_types.h"
#include "../commands/atlas_command.h"
#include "../logger/atlas_logger.h"

atlas_status_t atlas_alert_push_cmd_parse(const uint8_t *buf, uint16_t buf_len, uint16_t *push_rate)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    uint8_t push_found = 0;
    atlas_status_t status = ATLAS_OK;

    if (!buf || !buf_len)
        return ATLAS_INVALID_INPUT;

    if (!push_rate)
        return ATLAS_INVALID_INPUT;

    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted telemetry alert command");
        status = ATLAS_CORRUPTED_COMMAND;
	goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_EXT_PUSH_RATE && cmd->length == sizeof(uint16_t)) {
            memcpy(push_rate, cmd->value, sizeof(uint16_t));
            *push_rate = ntohs(*push_rate);
	    push_found = 1;
	    break;
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }
     
    if (!push_found) {
        ATLAS_LOGGER_ERROR("External push rate was not found in the telemetry alert request");
        status = ATLAS_CORRUPTED_COMMAND;
	goto EXIT;
    }

EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}

atlas_status_t atlas_alert_threshold_cmd_parse(const uint8_t *buf, uint16_t buf_len,
                                               uint16_t *scan_rate, char **threshold)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    uint8_t scan_found = 0;
    atlas_status_t status = ATLAS_OK;

    if (!buf || !buf_len)
        return ATLAS_INVALID_INPUT;

    if (!scan_rate || !threshold)
        return ATLAS_INVALID_INPUT;

    *threshold = NULL;
    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted telemetry alert command");
        status = ATLAS_CORRUPTED_COMMAND;
        goto ERR;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_INT_SCAN_RATE && cmd->length == sizeof(uint16_t)) {
            memcpy(scan_rate, cmd->value, sizeof(uint16_t));
            *scan_rate = ntohs(*scan_rate);
            scan_found = 1;
        } else if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_THRESHOLD && cmd->length > 0) { 
            *threshold = calloc(1, cmd->length + 1);
            memcpy(*threshold, cmd->value, cmd->length);
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }
     
    if (!scan_found) {
        ATLAS_LOGGER_ERROR("Internal scan was not found in the telemetry alert request");
        status = ATLAS_CORRUPTED_COMMAND;
        goto ERR;
    }
    if (!threshold) {
        ATLAS_LOGGER_ERROR("Threshold was not found in the telemetry alert request");
        status = ATLAS_CORRUPTED_COMMAND;
        goto ERR;
    }

    atlas_cmd_batch_free(cmd_batch);

    return ATLAS_OK;

ERR:
    free(*threshold);
    *threshold = NULL;
    
    atlas_cmd_batch_free(cmd_batch);

    return status;
}

