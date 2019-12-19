#include <stdio.h>
#include "coap_server/atlas_coap_server.h"

int main(int argc, char **argv)
{
    /* Start server */
    if (atlas_coap_server_start("127.0.0.1", "10000") != ATLAS_OK) {
        printf("Cannot start CoAP server");
        return -1;
    }

    /* Run server main loop */
    atlas_coap_server_loop();
        
    return 0;
}
