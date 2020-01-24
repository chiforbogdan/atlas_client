#include <string.h>
#include "atlas_register.h"
#include "../commands/atlas_command_types.h"
#include "../commands/atlas_command.h"
#include "../logger/atlas_logger.h"
#include "../identity/atlas_identity.h"
#include "../coap/atlas_coap_client.h"
#include "../coap/atlas_coap_response.h"
#include "../alarm/atlas_alarm.h"

#define ATLAS_CLIENT_REGISTER_TIMEOUT_MS (5000)
#define ATLAS_CLIENT_KEEPALIVE_TIMEOUT_MS (20000)
#define ATLAS_CLIENT_REGISTERED          (1)
#define ATLAS_CLIENT_NOT_REGISTERED      (0)
#define ATLAS_CLIENT_KEEPALIVE_COUNT     (3)

/* Indicates if the client is registered or not */
static uint8_t registered;
/* Keeps the keep-alive token */
static uint16_t ka_token;
/* Keeps the keep-alive count value (if this reaches 0, the register procedure is triggered again) */
static uint8_t ka_count;
/* Keeps the keep-alive alarm id */
static atlas_alarm_id_t ka_alarm_id;

static void reg_alarm_callback(atlas_alarm_id_t reg_alarm_id);
static void keepalive_alarm_callback(atlas_alarm_id_t reg_alarm_id);
static void reg_callback(atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len);
static void keepalive_callback(atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len);
static void send_keepalive_command();
static void send_register_command();

static void
keepalive_alarm_callback(atlas_alarm_id_t reg_alarm_id)
{
    ATLAS_LOGGER_INFO("Keep-alive alarm callback");

    send_keepalive_command();
}

static void
reg_alarm_callback(atlas_alarm_id_t reg_alarm_id)
{
    ATLAS_LOGGER_INFO("Registration alarm callback");

    send_register_command();
}

static void
keepalive_callback(atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len)
{
    uint16_t token;

    ATLAS_LOGGER_DEBUG("Keepalive callback executed");

    if (resp_status != ATLAS_COAP_RESP_OK) {
        /* Force registration immediately */
        if (resp_status == ATLAS_COAP_RESP_NOT_FOUND || resp_status == ATLAS_COAP_RESP_NOT_ACCEPTABLE_HERE)
            ka_count = 1;

        goto ERR;
    }

    /* Validate token */
    if (!resp_payload) {
        ATLAS_LOGGER_ERROR("Invalid keep-alive response payload");
        goto ERR;
    }

    if (resp_payload_len < sizeof(ka_token)) {
        ATLAS_LOGGER_ERROR("Invalid keep-alive response payload");
        goto ERR;
    }

    token = *((uint16_t *) resp_payload);

    if (ka_token != token) {
        ATLAS_LOGGER_ERROR("Keep-alive response token is NOT EQUAL with the local token");
        goto ERR;
    }

    ATLAS_LOGGER_INFO("Keepalive SUCCESS!");

    /* Start keep-alive timer */
    ka_count = ATLAS_CLIENT_KEEPALIVE_COUNT;

    return;

ERR:
    ATLAS_LOGGER_ERROR("Error in completing the keepalive request");
        
    ka_count--;
    if (!ka_count) {
        ATLAS_LOGGER_ERROR("Keep-alive count reached 0. Trigger the register procedure");
            
        registered = ATLAS_CLIENT_NOT_REGISTERED;
            
        atlas_alarm_cancel(ka_alarm_id);
        ka_alarm_id = -1;

        send_register_command();
    }
}

static void
reg_callback(atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len)
{
    ATLAS_LOGGER_DEBUG("Register callback executed");

    if (resp_status != ATLAS_COAP_RESP_OK) {
        ATLAS_LOGGER_ERROR("Error in completing the registration request");
        
        /* Start register timer */
        if (atlas_alarm_set(ATLAS_CLIENT_REGISTER_TIMEOUT_MS, reg_alarm_callback, ATLAS_ALARM_RUN_ONCE) < 0)
            ATLAS_LOGGER_ERROR("Error in scheduling a register alarm!");
        
        return;
    }

    ATLAS_LOGGER_INFO("Registration to gateway is COMPLETED!");

    registered = ATLAS_CLIENT_REGISTERED;
    
    /* Start keep-alive timer */
    ka_alarm_id = atlas_alarm_set(ATLAS_CLIENT_KEEPALIVE_TIMEOUT_MS,
                                  keepalive_alarm_callback,
                                  ATLAS_ALARM_RUN_MULTIPLE_TIMES);
    if (ka_alarm_id < 0)
        ATLAS_LOGGER_ERROR("Error in scheduling the keepalive alarm!");
}

static void
send_keepalive_command()
{
    atlas_cmd_batch_t *cmd_batch;
    atlas_status_t status;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    
    cmd_batch = atlas_cmd_batch_new();

    /* Add identity */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    
    /* Add keep-alive token */
    ka_token++;
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_KEEPALIVE, sizeof(ka_token), (uint8_t *) &ka_token);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    status = atlas_coap_client_request("coaps://127.0.0.1:10100/gateway/keepalive", ATLAS_COAP_METHOD_PUT,
                                       cmd_buf, cmd_len, ATLAS_CLIENT_REGISTER_TIMEOUT_MS,
                                       keepalive_callback);
    if (status != ATLAS_OK)
        ATLAS_LOGGER_ERROR("Error when sending REGISTER request");

    atlas_cmd_batch_free(cmd_batch);
}

static void
send_register_command()
{
    atlas_cmd_batch_t *cmd_batch;
    atlas_status_t status;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    
    cmd_batch = atlas_cmd_batch_new();

    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_REGISTER, strlen(identity), (uint8_t *)identity);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    status = atlas_coap_client_request("coaps://127.0.0.1:10100/gateway/register", ATLAS_COAP_METHOD_POST,
                                       cmd_buf, cmd_len, ATLAS_CLIENT_REGISTER_TIMEOUT_MS,
                                       reg_callback);
    if (status != ATLAS_OK)
        ATLAS_LOGGER_ERROR("Error when sending REGISTER request");

    atlas_cmd_batch_free(cmd_batch);
}

void
atlas_register_start()
{
    ATLAS_LOGGER_DEBUG("Register start...");

    send_register_command();
}
