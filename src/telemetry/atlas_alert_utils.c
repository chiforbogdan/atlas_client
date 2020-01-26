#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "atlas_alert_utils.h"
#include "../commands/atlas_command_types.h"
#include "../commands/atlas_command.h"
#include "../logger/atlas_logger.h"

atlas_status_t atlas_alert_cmd_parse(const uint8_t *buf, uint16_t buf_len,
                                     uint16_t **ext_push, uint16_t **int_scan, char **threshold)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    atlas_status_t status;

    if (!buf || !buf_len)
        return ATLAS_INVALID_INPUT;

    if (!ext_push || !int_scan || !threshold)
        return ATLAS_INVALID_INPUT;

    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted telemetry alert command");
        goto ERR;
    }

    *ext_push = NULL;
    *int_scan = NULL;
    *threshold = NULL;

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_EXT_PUSH_RATE &&
            cmd->length == sizeof(uint16_t)) {
            
            *ext_push = malloc(sizeof(uint16_t)); 
            memcpy(*ext_push, cmd->value, sizeof(uint16_t));
            **ext_push = ntohs(**ext_push);
        } else if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_INT_SCAN_RATE &&
                   cmd->length == sizeof(uint16_t)) {
        
            *int_scan = malloc(sizeof(uint16_t));    
            memcpy(*int_scan, cmd->value, sizeof(uint16_t));
            **int_scan = ntohs(**int_scan);
        } else if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_THRESHOLD &&
                   cmd->length > 0) {
            
            *threshold = calloc(1, cmd->length + 1);
            memcpy(*threshold, cmd->value, cmd->length);
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }
     
    if (!(*ext_push)) {
        ATLAS_LOGGER_ERROR("External push rate was not found in the telemetry alert request");
        goto ERR;
    }
    if (!(*int_scan) && (*threshold)) {
        ATLAS_LOGGER_ERROR("Internal scan was not found while threshold was found in the telemetry alert request");
        goto ERR;
    }
    if (!(*threshold) && (*int_scan)) {
        ATLAS_LOGGER_ERROR("Threshold was not found while internal scan was foudn in the telemetry alert request");
        goto ERR;
    }


    return ATLAS_OK;

ERR:
    free(*ext_push);
    *ext_push = NULL;

    free(*int_scan);
    *int_scan = NULL;
    
    free(*threshold);
    *threshold = NULL;
    
    atlas_cmd_batch_free(cmd_batch);

    return ATLAS_CORRUPTED_COMMAND;
}

