#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../identity/atlas_identity.h"
#include "../coap/atlas_coap_server.h"
#include "../utils/atlas_config.h"
#include "../data_plane_connector/atlas_data_plane_connector.h"
#include "atlas_telemetry.h"
#include "atlas_alert_utils.h"
#include "atlas_telemetry_packets_info.h"

#define ATLAS_PACKETS_INFO_FEATURE_MAX_LEN (32)
#define ATLAS_PACKETS_INFO_PACKETS_PER_MINUTE "gateway/telemetry/packets_info/packets_per_minute"
#define ATLAS_PACKETS_INFO_PACKETS_AVG "gateway/telemetry/packets_info/packets_avg"

static void
atlas_telemetry_payload_packets_per_minute(uint8_t **payload, uint16_t *payload_len,
                               uint8_t use_threshold)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    uint16_t ppm;

    ATLAS_LOGGER_INFO("Get payload for packets_per_minute telemetry feature");

    if (use_threshold == ATLAS_TELEMETRY_USE_THRESHOLD) {
        ATLAS_LOGGER_ERROR("Packets_per_minute telemetry feature does not support thresholds");
        printf("Packets_per_minute telemetry feature does not support thresholds\n");
        return;
    }

    ppm = get_packets_per_min();

    /* Add packets_info packets_per_minute command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    
    ppm = htons(ppm);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_PACKETS_PER_MINUTE, sizeof(ppm), (uint8_t *)&ppm);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_packets_avg(uint8_t **payload, uint16_t *payload_len,
                               uint8_t use_threshold)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    uint16_t pack_avg;

    ATLAS_LOGGER_INFO("Get payload for sysinfo uptime telemetry feature");

    if (use_threshold == ATLAS_TELEMETRY_USE_THRESHOLD) {
        ATLAS_LOGGER_ERROR("Packets_avg telemetry feature does not support thresholds");
        printf("Packets_avg telemetry feature does not support thresholds\n");
        return;
    }

    pack_avg = get_packets_avg();
    
    /* Add packets_info packets_per_minute command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    
    pack_avg = htons(pack_avg);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_PACKETS_AVG, sizeof(pack_avg), (uint8_t *)&pack_avg);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

void 
atlas_telemetry_add_packets_info(){
    
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    ATLAS_LOGGER_DEBUG("Add packets_info telemetry feature");

    /* Add packets_info telemetry features */
    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_PER_MINUTE, uri);
    atlas_telemetry_add(uri, atlas_telemetry_payload_packets_per_minute);
    
    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_AVG, uri);
    atlas_telemetry_add(uri, atlas_telemetry_payload_packets_avg);
}
