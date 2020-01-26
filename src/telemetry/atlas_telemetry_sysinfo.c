#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include "atlas_telemetry_sysinfo.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../identity/atlas_identity.h"
#include "../coap/atlas_coap_server.h"
#include "atlas_telemetry.h"

#define ATLAS_SYSINFO_FEATURE_MAX_LEN (32)

static void
atlas_telemetry_payload_uptime(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char uptime[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo uptime telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo uptime");
        return;
    }

    sprintf(uptime, "%lu",info.uptime);

    /* Add sysinfo uptime command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_UPTIME, strlen(uptime), (uint8_t *)uptime);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_totalram(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char totalram[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo totalram telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo totalram");
        return;
    }

    sprintf(totalram, "%lu",info.totalram);

    /* Add sysinfo totalram command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_TOTALRAM, strlen(totalram), (uint8_t *)totalram);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_freeram(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char freeram[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo freeram telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo freeram");
        return;
    }

    sprintf(freeram, "%lu",info.freeram);

    /* Add sysinfo freeram command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_FREERAM, strlen(freeram), (uint8_t *)freeram);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_sharedram(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char sharedram[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo sharedram telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo sharedram");
        return;
    }

    sprintf(sharedram, "%lu",info.sharedram);

    /* Add sysinfo sharedram command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_SHAREDRAM, strlen(sharedram), (uint8_t *)sharedram);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_bufferram(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char bufferram[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo bufferram telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo bufferram");
        return;
    }

    sprintf(bufferram, "%lu",info.bufferram);

    /* Add sysinfo bufferram command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_BUFFERRAM, strlen(bufferram), (uint8_t *)bufferram);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_totalswap(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char totalswap[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo totalswap telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo totalswap");
        return;
    }

    sprintf(totalswap, "%lu",info.totalswap);

    /* Add sysinfo totalswap command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_TOTALSWAP, strlen(totalswap), (uint8_t *)totalswap);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_freeswap(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char freeswap[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo freeswap telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo freeswap");
        return;
    }

    sprintf(freeswap, "%lu",info.freeswap);

    /* Add sysinfo uptime command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_FREESWAP, strlen(freeswap), (uint8_t *)freeswap);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_procs(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char procs[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo procs telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo procs");
        return;
    }

    sprintf(procs, "%u",info.procs);

    /* Add sysinfo procs command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_PROCS, strlen(procs), (uint8_t *)procs);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_totalhigh(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char totalhigh[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo totalhigh telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo totalhigh");
        return;
    }

    sprintf(totalhigh, "%lu",info.totalhigh);

    /* Add sysinfo totalhigh command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_TOTALHIGH, strlen(totalhigh), (uint8_t *)totalhigh);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_freehigh(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char freehigh[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo freehigh telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo freehigh");
        return;
    }

    sprintf(freehigh, "%lu",info.freehigh);

    /* Add sysinfo freehigh command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_FREEHIGH, strlen(freehigh), (uint8_t *)freehigh);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_load1(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char load1[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo load1 telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo load1");
        return;
    }

    sprintf(load1, "%lu",info.loads[0]);

    /* Add sysinfo load1 command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_LOAD1, strlen(load1), (uint8_t *)load1);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_load5(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char load5[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo load5 telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo load5");
        return;
    }

    sprintf(load5, "%lu",info.loads[1]);

    /* Add sysinfo load5 command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_LOAD5, strlen(load5), (uint8_t *)load5);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_telemetry_payload_load15(uint8_t **payload, uint16_t *payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    struct sysinfo info;
    char load15[ATLAS_SYSINFO_FEATURE_MAX_LEN + 1] = { 0 };

    ATLAS_LOGGER_INFO("Get payload for sysinfo load15 telemetry feature");

    if (sysinfo(&info) != 0) {
        ATLAS_LOGGER_ERROR("Error in getting sysinfo load15");
        return;
    }

    sprintf(load15, "%lu",info.loads[2]);

    /* Add sysinfo load15 command */
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_TELEMETRY_SYSINFO_LOAD15, strlen(load15), (uint8_t *)load15);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    *payload = malloc(cmd_len);
    memcpy(*payload, cmd_buf, cmd_len);
    *payload_len = cmd_len;

    atlas_cmd_batch_free(cmd_batch);
}

atlas_coap_response_t
atlas_telemetry_alert_cb(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len,
                         uint8_t **resp_payload, size_t *resp_payload_len)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    atlas_status_t status;
    uint16_t ext_push, int_scan;
    char *threshold = NULL;
    uint8_t ext_push_found = 0, int_scan_found = 0;

    ATLAS_LOGGER_DEBUG("Telemetry sysinfo alert end-point called");

    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, req_payload, req_payload_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted command received on the telemetry sysinfo alert end-point");
        goto ERR;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_EXT_PUSH_RATE && cmd->length == sizeof(uint16_t)) {
            memcpy(&ext_push, cmd->value, sizeof(uint16_t));
            ext_push = ntohs(ext_push);
            ext_push_found = 1;
        } else if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_INT_SCAN_RATE && cmd->length == sizeof(uint16_t)) {
            memcpy(&int_scan, cmd->value, sizeof(uint16_t));
            int_scan = ntohs(int_scan);
            int_scan_found = 1;
        } else if (cmd->type == ATLAS_CMD_TELEMETRY_ALERT_THRESHOLD && cmd->length > 0) {
            threshold = calloc(1, cmd->length + 1);
            memcpy(threshold, cmd->value, cmd->length);
        }
        
        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }

    if (!ext_push_found) {
        ATLAS_LOGGER_ERROR("External push rate was not found in the telemetry alert request");
        goto ERR;
    }
    if (!int_scan_found && threshold) {
        ATLAS_LOGGER_ERROR("Internal scan was not found while threshold was found in the telemetry alert request");
        goto ERR;
    }
    if (!threshold && int_scan_found) {
        ATLAS_LOGGER_ERROR("Threshold was not found while internal scan was foudn in the telemetry alert request");
        goto ERR;
    }

    /* TODO install alert */

    free(threshold);

    return ATLAS_COAP_RESP_OK;

ERR:
    free(threshold);
    atlas_cmd_batch_free(cmd_batch);
    
    return ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE;
}

void
atlas_telemetry_add_sysinfo()
{
    atlas_status_t status;

    ATLAS_LOGGER_DEBUG("Add sysinfo telemetry feature");

    /* Add sysinfo telemetry features */
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/uptime", atlas_telemetry_payload_uptime);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/totalram", atlas_telemetry_payload_totalram);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/freeram", atlas_telemetry_payload_freeram);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/sharedram", atlas_telemetry_payload_sharedram);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/bufferram", atlas_telemetry_payload_bufferram);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/totalswap", atlas_telemetry_payload_totalswap);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/freeswap", atlas_telemetry_payload_freeswap);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/procs", atlas_telemetry_payload_procs);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/totalhigh", atlas_telemetry_payload_totalhigh);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/freehigh", atlas_telemetry_payload_freehigh);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/load1", atlas_telemetry_payload_load1);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/load5", atlas_telemetry_payload_load5);
    atlas_telemetry_add("coaps://127.0.0.1:10100/gateway/telemetry/sysinfo/load15", atlas_telemetry_payload_load15);

    /* Add sysinfo telemetry alerts */
    status = atlas_coap_server_add_resource("client/telemetry/alerts/sysinfo/procs", ATLAS_COAP_METHOD_PUT, atlas_telemetry_alert_cb);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Cannot install sysinfo procs telemetry alert end-point");
        return;
    }
}
