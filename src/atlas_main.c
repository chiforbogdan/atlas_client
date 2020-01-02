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

atlas_coap_response_t test_get(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len, uint8_t **resp_payload, size_t *resp_payload_len)
{
    int i;

    printf("GET URI PATH: %s\n", uri_path);

    if (req_payload && req_payload_len) {
        printf("Payload:\n");
        for (i = 0; i < req_payload_len; i++)
            printf("%c", req_payload[i]);
    }

    printf("\n");
    *resp_payload = malloc(30);
    memset(*resp_payload, 'A', 30);
    *resp_payload_len = 30;

    return ATLAS_COAP_RESP_OK;
}

atlas_coap_response_t test_post(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len, uint8_t **resp_payload, size_t *resp_payload_len)
{
    int i;
 
    printf("POST URI PATH: %s\n", uri_path);
 
    if (req_payload && req_payload_len) {
        printf("Payload:\n");
            for (i = 0; i < req_payload_len; i++)
                printf("%c", req_payload[i]);
    }
 
    printf("\n");
    *resp_payload = malloc(30);
    memset(*resp_payload, 'B', 30);
    *resp_payload_len = 30;
 
    return ATLAS_COAP_RESP_OK;
 }

atlas_coap_response_t test_put(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len, uint8_t **resp_payload, size_t *resp_payload_len)
{
    int i;

    printf("PUT URI PATH: %s\n", uri_path);

    if (req_payload && req_payload_len) {
        printf("Payload:\n");
        for (i = 0; i < req_payload_len; i++)
            printf("%c", req_payload[i]);
    }
 
    printf("\n");
    *resp_payload = malloc(30);
    memset(*resp_payload, 'C', 30);
    *resp_payload_len = 30;
 
    return ATLAS_COAP_RESP_OK;
 }

atlas_coap_response_t test_del(const char *uri_path, const uint8_t *req_payload, size_t req_payload_len, uint8_t **resp_payload, size_t *resp_payload_len)
{
    int i;

    printf("GET URI PATH: %s\n", uri_path);

    if (req_payload && req_payload_len) {
        printf("Payload:\n");
        for (i = 0; i < req_payload_len; i++)
            printf("%c", req_payload[i]);
    }
    printf("\n");
 
    *resp_payload = malloc(30);
    memset(*resp_payload, 'D', 30);
    *resp_payload_len = 30;
 
    return ATLAS_COAP_RESP_OK;
 }

int main(int argc, char **argv)
{
    atlas_log_init();

    ATLAS_LOGGER_INFO("Starting ATLAS IoT client...");

    /* Start server */
    if (atlas_coap_server_start("127.0.0.1", "10001") != ATLAS_OK) {
        ATLAS_LOGGER_INFO("Cannot start CoAP server");
        return -1;
    }

    ATLAS_LOGGER_INFO("CoAP server started");

    uint8_t p[] = {'A', 'B', 'C', 'D'};

    atlas_coap_client_request("coap://127.0.0.1:10000/test_put/put1", ATLAS_COAP_METHOD_PUT, p, sizeof(p), 11000, test);

    //atlas_coap_server_add_resource("test_get/get", ATLAS_COAP_METHOD_GET, test_get);
    //atlas_coap_server_add_resource("test_post/post", ATLAS_COAP_METHOD_POST, test_post);
    //atlas_coap_server_add_resource("test_put/put", ATLAS_COAP_METHOD_PUT, test_put);
    //atlas_coap_server_add_resource("test_del/del", ATLAS_COAP_METHOD_DELETE, test_del);

    /* Run scheduler main loop */
    atlas_sched_loop();

    ATLAS_LOGGER_INFO("Stopping ATLAS IoT client...");
    
    atlas_log_close();

    return 0;
}
