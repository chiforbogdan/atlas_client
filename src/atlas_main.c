#include <stdio.h>
#include "coap/atlas_coap_server.h"
#include "logger/atlas_logger.h"
#include "scheduler/atlas_scheduler.h"
#include "alarm/atlas_alarm.h"
#include "coap/atlas_coap_response.h"
#include "coap/atlas_coap_client.h"

void test (atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len)
{
    int i;

    printf("status %d\n", resp_status);

    if (resp_payload && resp_payload_len)
    for (i = 0; i < resp_payload_len; i++)
        printf("%c", resp_payload[i]);

    printf("\n");
}

int main(int argc, char **argv)
{
    atlas_log_init();

    ATLAS_LOGGER_INFO("Starting ATLAS IoT client...");

    /* Start server */
    if (atlas_coap_server_start("127.0.0.1", "10000") != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot start CoAP server");
        return -1;
    }

    ATLAS_LOGGER_INFO("CoAP server started");

    uint8_t p[] = {1,2,3,4,5,6,7,8};

    atlas_coap_client_request("127.0.0.1", 5683, p, sizeof(p), test);

    /* Run scheduler main loop */
    atlas_sched_loop();

    ATLAS_LOGGER_INFO("Stopping ATLAS IoT client...");
    
    atlas_log_close();

    return 0;
}
