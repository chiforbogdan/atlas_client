#include <stdio.h>
#include "coap/atlas_coap_server.h"
#include "logger/atlas_logger.h"
#include "scheduler/atlas_scheduler.h"
#include "alarm/atlas_alarm.h"
#include "coap/atlas_coap_response.h"

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

    /* Run scheduler main loop */
    atlas_sched_loop();

    ATLAS_LOGGER_INFO("Stopping ATLAS IoT client...");
    
    atlas_log_close();

    return 0;
}
