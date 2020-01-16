#include <stdio.h>
#include "coap/atlas_coap_server.h"
#include "logger/atlas_logger.h"
#include "scheduler/atlas_scheduler.h"
#include "alarm/atlas_alarm.h"
#include "coap/atlas_coap_response.h"
#include "coap/atlas_coap_client.h"
#include "identity/atlas_identity.h"
#include "register/atlas_register.h"

int main(int argc, char **argv)
{
    atlas_log_init();

    ATLAS_LOGGER_INFO("Starting ATLAS IoT client...");

    /* Init or generate identity */
    if (atlas_identity_init() != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot start client - identity error");
        return -1;
    }

    /* Set identity info for the CoAP client */
    if (atlas_coap_client_set_dtls_info(atlas_identity_get(), atlas_psk_get()) != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot set client DTLS info");
        return -1;
    }

    /* Init registration and keepalive */
    atlas_register_start();

    /* Start server */
    if (atlas_coap_server_start("127.0.0.1", "10001", ATLAS_COAP_SERVER_MODE_BOTH, "12345") != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot start CoAP server");
        return -1;
    }

    ATLAS_LOGGER_INFO("CoAP server started");

    /* Run scheduler main loop */
    atlas_sched_loop();

    ATLAS_LOGGER_INFO("Stopping ATLAS IoT client...");
    
    atlas_log_close();

    return 0;
}
