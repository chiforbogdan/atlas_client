#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "coap/atlas_coap_server.h"
#include "logger/atlas_logger.h"
#include "scheduler/atlas_scheduler.h"
#include "alarm/atlas_alarm.h"
#include "coap/atlas_coap_response.h"
#include "coap/atlas_coap_client.h"
#include "identity/atlas_identity.h"
#include "register/atlas_register.h"
#include "telemetry/atlas_telemetry_features.h"
#include "utils/atlas_config.h"
#include "received_commands/received_commands.h"

static void
print_usage()
{
    printf("Usage: atlas_client -h <hostname> -p <port> -i <interface> -l <port>\n");
}

static void
parse_options(int argc, char **argv)
{
    int opt;
    uint8_t hostname_opt = 0;
    uint8_t gw_port_opt = 0;
    uint8_t iface_opt = 0;
    uint8_t local_port_opt = 0;
  
    while((opt = getopt(argc, argv, ":h:p:i:l:")) != -1) {  
        switch(opt)  {
            case 'h':
                if (atlas_cfg_set_hostname(optarg) != ATLAS_OK) {
                    print_usage();
                    exit(1);
                }
                hostname_opt = 1;
                
                break;  
            case 'p':  
                if (atlas_cfg_set_port(optarg) != ATLAS_OK) {
                    print_usage();
                    exit(1);
                }
                gw_port_opt = 1;
                
                break;  
            case 'i':  
                if (atlas_cfg_set_local_iface(optarg) != ATLAS_OK) {
                    print_usage();
                    exit(1);
                }
                iface_opt = 1;

                break;  
            case 'l':  
                if (atlas_cfg_set_local_port(optarg) != ATLAS_OK) {
                    print_usage();
                    exit(1);
                }
                local_port_opt = 1;

                break;  
 
            default:  
                printf("unknown option: %c\n", optopt);
                print_usage();
                exit(1);
        }  
    }
    
    if (!hostname_opt || !gw_port_opt || !iface_opt || !local_port_opt) {
        print_usage();
        exit(1);
    }
}

int
main(int argc, char **argv)
{
    uint16_t local_port;

    parse_options(argc, argv);
    
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
    local_port = atlas_cfg_get_local_port();
    if (atlas_coap_server_start(local_port, ATLAS_COAP_SERVER_MODE_DTLS_PSK, atlas_psk_get()) != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot start CoAP server");
        return -1;
    }

    ATLAS_LOGGER_INFO("CoAP server started");

    /* Init commands receiver modules */
    if (atlas_receive_commands_start() != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot start receive commands module (data plane agent)");
	return -1;
    }
   
    /* Init telemetry features */
    atlas_telemetry_features_init();

    /* Run scheduler main loop */
    atlas_sched_loop();

    ATLAS_LOGGER_INFO("Stopping ATLAS IoT client...");
    
    atlas_log_close();

    return 0;
}
