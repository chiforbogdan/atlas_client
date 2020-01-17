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

static uint8_t registered;

static void reg_alarm_callback(atlas_alarm_id_t reg_alarm_id);
static void reg_callback(atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len);
static void send_register_command();

static void
reg_alarm_callback(atlas_alarm_id_t reg_alarm_id)
{
    ATLAS_LOGGER_INFO("Registration alarm callback");

    send_register_command();
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

    status = atlas_coap_client_request("coaps://127.0.0.1/gateway/register", ATLAS_COAP_METHOD_PUT,
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
