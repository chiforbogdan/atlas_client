#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "atlas_telemetry_hostname.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../identity/atlas_identity.h"
#include "atlas_telemetry.h"
#include "../utils/atlas_config.h"

#define ATLAS_HOSTNAME_FILE "/etc/hostname"
#define ATLAS_HOSTNAME_MAX_LEN (32)
#define ATLAS_HOSTNAME_PATH "gateway/telemetry/hostname"

static void
atlas_telemetry_payload_hostname(uint8_t **payload, uint16_t *payload_len,
                                 uint8_t use_threshold)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    char hostname[ATLAS_HOSTNAME_MAX_LEN + 1] = { 0 };
    const char *identity = atlas_identity_get();
    int fd;
    int len;
    int i;

    ATLAS_LOGGER_INFO("Get payload for hostname telemetry feature"); 

    if (use_threshold == ATLAS_TELEMETRY_USE_THRESHOLD) {
        ATLAS_LOGGER_ERROR("Telemetry hostname feature does not support thresholds");
        return;	
    }

    fd = open(ATLAS_HOSTNAME_FILE, O_RDONLY);
    if (fd < 0) {
        ATLAS_LOGGER_ERROR("Error when reading hostname");
        return;
    }

    len = read(fd, hostname, sizeof(hostname) - 1);
    close (fd);

    if (len  <= 0)
        return;

    /* Remove \n from hostname */
    for (i = strlen(hostname) - 1; i >= 0; i--) {
        if (hostname[i] == '\n') {
            hostname[i] = 0;
            break;
        }
    }

    /* Add hostname command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_HOSTNAME, strlen(hostname), (uint8_t *)hostname);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

void
atlas_telemetry_add_hostname()
{
    ATLAS_LOGGER_DEBUG("Add hostname telemetry feature");
    
    char uri[ATLAS_URI_MAX_LEN] = { 0 };
    
    atlas_cfg_coap_get_uri(ATLAS_HOSTNAME_PATH, uri);

    atlas_telemetry_add(uri, atlas_telemetry_payload_hostname);
}

