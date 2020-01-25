#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include "atlas_telemetry_kern_info.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../identity/atlas_identity.h"
#include "atlas_telemetry.h"

#define ATLAS_KERNEL_INFO_MAX_LEN (128)

static void
atlas_telemetry_payload_kern_info(uint8_t **payload, uint16_t *payload_len)
{   
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct utsname buffer;
    char kern_info[ATLAS_KERNEL_INFO_MAX_LEN + 1] = { 0 };
    
    ATLAS_LOGGER_INFO("Get payload for kernel info telemetry feature");
    
    if (uname(&buffer) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting kernel information");
        return;
    }
    
    strncpy(kern_info, buffer.sysname, sizeof(kern_info) - 1);
    strncat(kern_info, " ", sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, buffer.nodename, sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, " ", sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, buffer.release, sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, " ", sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, buffer.version, sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, " ", sizeof(kern_info) - strlen(kern_info) - 1);
    strncat(kern_info, buffer.machine, sizeof(kern_info) - strlen(kern_info) - 1);
    
    /* Add kernel info command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_KERN_INFO, strlen(kern_info), (uint8_t *)kern_info);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    
    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;
    
    atlas_cmd_batch_free(cmd_batch);
}

void
atlas_telemetry_add_kern_info()
{
    ATLAS_LOGGER_DEBUG("Add kernel info telemetry feature");

    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/kernel_info", atlas_telemetry_payload_kern_info);
}

