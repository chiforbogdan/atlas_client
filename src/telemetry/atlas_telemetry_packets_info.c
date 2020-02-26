#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <limits.h>
#include <errno.h>
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
#define ATLAS_PACKETS_INFO_PACKETS_PER_MINUTE "gateway/telemetry/packets_per_minute"
#define ATLAS_PACKETS_INFO_PACKETS_AVG "gateway/telemetry/packets_avg"

static uint16_t procs_threshold;

static void
atlas_telemetry_payload_packets_per_minute(uint8_t **payload, uint16_t *payload_len,
                               uint8_t use_threshold)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    uint16_t ppm;
    char str[ATLAS_PACKETS_INFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for packets_per_minute telemetry feature");
    
    ppm = get_packets_per_min();
    sprintf(str, "%u",ppm);

    /* Add packets_info packets_per_minute command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_PACKETS_PER_MINUTE, strlen(str), (uint8_t *)str);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static atlas_coap_response_t
atlas_threshold_alert_ppm_cb(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len,
                               uint8_t **resp_payload, size_t *resp_payload_len)
{
    atlas_status_t status;
    uint16_t scan_rate;
    char *threshold = NULL;
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    ATLAS_LOGGER_DEBUG("Telemetry ppm threshold alert end-point called");
    
    status = atlas_alert_threshold_cmd_parse(req_payload, req_payload_len, &scan_rate,
                                             &threshold);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Telemetry ppm threshold alert end-point encountered an error when parsing the command");
        return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
    }
    
    errno = 0;
    procs_threshold = strtol(threshold, NULL, 10);

    free(threshold);
   
    /* Check for various possible errors */
    if ((errno == ERANGE && (procs_threshold == LONG_MAX || procs_threshold == LONG_MIN))
            || (errno != 0 && procs_threshold == 0)) {
        ATLAS_LOGGER_ERROR("Telemetry ppm alert end-point encountered an error when parsing the threshold value");
        return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
    }

    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_PER_MINUTE, uri);
    atlas_telemetry_threshold_set(uri, scan_rate);

    return ATLAS_COAP_RESP_OK;
}

static atlas_coap_response_t
atlas_threshold_alert_avg_cb(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len,
                               uint8_t **resp_payload, size_t *resp_payload_len)
{
    atlas_status_t status;
    uint16_t scan_rate;
    char *threshold = NULL;
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    ATLAS_LOGGER_DEBUG("Telemetry packets avg threshold alert end-point called");

    status = atlas_alert_threshold_cmd_parse(req_payload, req_payload_len, &scan_rate,
                                             &threshold);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Telemetry packets avg threshold alert end-point encountered an error when parsing the command");
        return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
    }
    
    errno = 0;
    procs_threshold = strtol(threshold, NULL, 10);

    free(threshold);
   
    /* Check for various possible errors */
    if ((errno == ERANGE && (procs_threshold == LONG_MAX || procs_threshold == LONG_MIN))
            || (errno != 0 && procs_threshold == 0)) {
        ATLAS_LOGGER_ERROR("Telemetry packets avg alert end-point encountered an error when parsing the threshold value");
        return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
    }

    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_AVG, uri);
    atlas_telemetry_threshold_set(uri, scan_rate);

    return ATLAS_COAP_RESP_OK;
}

static atlas_coap_response_t
atlas_push_alert_ppm_cb(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len,
                          uint8_t **resp_payload, size_t *resp_payload_len)
{
    atlas_status_t status;
    uint16_t push_rate;
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    ATLAS_LOGGER_DEBUG("Telemetry ppm push alert end-point called");

    status = atlas_alert_push_cmd_parse(req_payload, req_payload_len, &push_rate);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_DEBUG("Telemetry ppm push alert end-point encountered an error when parsing the command");
        return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
    }

    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_PER_MINUTE, uri);
       
    atlas_telemetry_push_set(uri, push_rate);
 
    return ATLAS_COAP_RESP_OK;
}

static atlas_coap_response_t
atlas_push_alert_avg_cb(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len,
                          uint8_t **resp_payload, size_t *resp_payload_len)
{
    atlas_status_t status;
    uint16_t push_rate;
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    ATLAS_LOGGER_DEBUG("Telemetry packets avg push alert end-point called");    

    status = atlas_alert_push_cmd_parse(req_payload, req_payload_len, &push_rate);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_DEBUG("Telemetry packets avg push alert end-point encountered an error when parsing the command");
        return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
    }
    

    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_AVG, uri);
       
    atlas_telemetry_push_set(uri, push_rate);
 
    return ATLAS_COAP_RESP_OK;
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
    char str[ATLAS_PACKETS_INFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo uptime telemetry feature");

    pack_avg = get_packets_avg();
    sprintf(str, "%u",pack_avg);
    
    /* Add packets_info packets_avg command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    
    pack_avg = htons(pack_avg);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_PACKETS_AVG, strlen(str), (uint8_t *)str);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

void 
atlas_telemetry_add_packets_info(){
    
    atlas_status_t status;
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    ATLAS_LOGGER_DEBUG("Add packets_info telemetry feature");

    /* Add packets_info telemetry features */
    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_PER_MINUTE, uri);
    atlas_telemetry_add(uri, atlas_telemetry_payload_packets_per_minute);
    
    atlas_cfg_coap_get_uri(ATLAS_PACKETS_INFO_PACKETS_AVG, uri);
    atlas_telemetry_add(uri, atlas_telemetry_payload_packets_avg);

    /* Add ppm alerts */
    status = atlas_coap_server_add_resource("client/telemetry/packets_info/packets_per_min/alerts/push", ATLAS_COAP_METHOD_PUT,
                                            atlas_push_alert_ppm_cb);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Cannot install ppm push telemetry alert end-point");
        return;
    }
    
    status = atlas_coap_server_add_resource("client/telemetry/packets_info/packets_avg/alerts/push", ATLAS_COAP_METHOD_PUT,
                                            atlas_push_alert_avg_cb);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Cannot install packets avg push telemetry alert end-point");
        return;
    }
    
    /* Add ppm threshold alerts */
    status = atlas_coap_server_add_resource("client/telemetry/packets_info/packets_per_min/alerts/threshold", ATLAS_COAP_METHOD_PUT,
                                            atlas_threshold_alert_ppm_cb);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Cannot install ppm threshold telemetry alert end-point");
        return;
    }
    
    status = atlas_coap_server_add_resource("client/telemetry/packets_info/packets_avg/alerts/threshold", ATLAS_COAP_METHOD_PUT,
                                            atlas_threshold_alert_avg_cb);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Cannot install packets avg threshold telemetry alert end-point");
        return;
    }
}
