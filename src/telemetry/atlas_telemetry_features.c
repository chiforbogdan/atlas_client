#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>
#include "atlas_telemetry_features.h"
#include "atlas_telemetry.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command_types.h"
#include "../commands/atlas_command.h"
#include "../identity/atlas_identity.h"

#define ATLAS_HOSTNAME_FILE "/etc/hostname"
#define ATLAS_HOSTNAME_MAX_LEN (32)
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

static void
atlas_telemetry_payload_hostname(uint8_t **payload, uint16_t *payload_len)
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

static void
atlas_telemetry_add_hostname()
{
    ATLAS_LOGGER_DEBUG("Add hostname telemetry feature");

    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/hostname", atlas_telemetry_payload_hostname);
}

static void
atlas_telemetry_add_kern_info()
{
    ATLAS_LOGGER_DEBUG("Add kernel info telemetry feature");

    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/kernel_info", atlas_telemetry_payload_kern_info);
}

void
atlas_telemetry_features_init()
{
    ATLAS_LOGGER_DEBUG("Init telemetry features...");

    /* Add hostname feature */
    atlas_telemetry_add_hostname();

    /* Add kernel info feature */
    atlas_telemetry_add_kern_info();
}
