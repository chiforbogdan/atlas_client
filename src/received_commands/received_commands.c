#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "../scheduler/atlas_scheduler.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../utils/atlas_config.h"
#include "../coap/atlas_coap_client.h"
#include "../coap/atlas_coap_response.h"
#include "../alarm/atlas_alarm.h"
#include "received_commands.h"

#define ATLAS_CLIENT_POLICY_TIMEOUT_MS  (5000)
#define ATLAS_CLIENT_POLICY_COAP_PATH   "gateway/policy"


char *socket_path = "\0hidden";
struct sockaddr_un addr;
char buf[100];
char *buffer;
int fd,cl,rc;

uint8_t* username;
uint8_t* clientid;
uint16_t policy_packets_per_min;
uint16_t policy_packets_avg;

static void send_policy_command();
static void policy_alarm_callback();
static void policy_callback(const char *uri, atlas_coap_response_t resp_status, const uint8_t *resp_payload, size_t resp_payload_len);


static void set_username(const uint8_t *user){
    username = (uint8_t *)user;
}
static void set_clientid(const uint8_t *id){
    clientid = (uint8_t *)id;
}
static void set_policy_packets_per_min(const uint8_t* ppm){
    policy_packets_per_min = ppm[0];
}
static void set_policy_packets_avg(const uint8_t* pack_avg){
    policy_packets_avg= pack_avg[0];
}
static uint8_t* get_username(){
    return username;
}
static uint8_t* get_clientid(){
    return clientid;
}
static uint16_t get_policy_packets_per_min(){
    return policy_packets_per_min;
}
static uint16_t get_policy_packets_avg(){
    return policy_packets_avg;
}

static void
policy_alarm_callback()
{
    ATLAS_LOGGER_INFO("Policy alarm callback");
    printf("Policy alarm callback\n");
    send_policy_command();
}

static void 
policy_callback(const char *uri, atlas_coap_response_t resp_status,
         const uint8_t *resp_payload, size_t resp_payload_len)
{
    ATLAS_LOGGER_DEBUG("Policy callback executed");
    printf("Policy callback executed\n");

    if (resp_status != ATLAS_COAP_RESP_OK) {
        ATLAS_LOGGER_ERROR("Error in sending the policy values");
        printf("Error in sending the policy values\n");
        
        
        /* Start sending policy timer */
        if (atlas_alarm_set(ATLAS_CLIENT_POLICY_TIMEOUT_MS, policy_alarm_callback, ATLAS_ALARM_RUN_ONCE) < 0)
            ATLAS_LOGGER_ERROR("Error in scheduling a sending policy alarm!");
        
        return;
    }

    ATLAS_LOGGER_INFO("Sending policy to gateway is COMPLETED!");
}

static void
send_policy_command()
{
    atlas_cmd_batch_t *cmd_batch;
    atlas_status_t status;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;

    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    cmd_batch = atlas_cmd_batch_new();

    /* Add MQTT clientID */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_CLIENTID, strlen((char*)username), username);

    /* Add policy packets per minute value */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE, sizeof(policy_packets_per_min), (uint8_t *) &policy_packets_per_min);

    /* Add policy packets avg value */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_AVG, sizeof(policy_packets_avg), (uint8_t *) &policy_packets_avg);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    atlas_cfg_coap_get_uri(ATLAS_CLIENT_POLICY_COAP_PATH, uri);
    status = atlas_coap_client_request(uri, ATLAS_COAP_METHOD_PUT,
                                       cmd_buf, cmd_len, ATLAS_CLIENT_POLICY_TIMEOUT_MS,
                                       policy_callback);
    if (status != ATLAS_OK)
        ATLAS_LOGGER_ERROR("Error when sending policy request");

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_data_plane_callback(int fd)
{ 
    atlas_status_t status = ATLAS_OK;

    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      ATLAS_LOGGER_ERROR("Socket accept error");
    }

    if ( (rc=read(cl,buf,sizeof(buf))) > 0) {

        atlas_cmd_batch_t *cmd_batch;
        const atlas_cmd_t *cmd;
        
        cmd_batch = atlas_cmd_batch_new();
        status = atlas_cmd_batch_set_raw(cmd_batch, (uint8_t*)buf, rc);
        
        if (status != ATLAS_OK) {
            ATLAS_LOGGER_ERROR("Corrupted command from data plane");
            status = ATLAS_CORRUPTED_COMMAND;
        }
        
        cmd = atlas_cmd_batch_get(cmd_batch, NULL);
        while (cmd) {
            if (cmd->type == ATLAS_CMD_DATA_PLANE_USERNAME ) {
                set_username(cmd->value);
                printf("Am primit USERNAME %s\n", get_username());
            }
            else 
                if (cmd->type == ATLAS_CMD_DATA_PLANE_CLIENTID ) {
                    set_clientid(cmd->value);
                    printf("Am primit CLIENTID %s \n", get_clientid());
                }
                else
                    if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE ) {
                        set_policy_packets_per_min(cmd->value);
                        printf("Am primit POLICY PACKETS_PER_MINUTE %d \n", get_policy_packets_per_min());
                    }
                    else
                        if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_AVG ) {
                            set_policy_packets_avg(cmd->value);
                            printf("Am primit POLICY PACKETS_AVG %d\n", get_policy_packets_avg());
                        }
                        else
                            if (cmd->type == ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE ) {
                                printf("Am primit PACKETS_PER_MINUTE\n");
                            }
                            else
                            if (cmd->type == ATLAS_CMD_DATA_PLANE_PACKETS_AVG ) {
                                printf("Am primit PACKETS_AVG\n");
                            }

            cmd = atlas_cmd_batch_get(cmd_batch, cmd);
        }
        atlas_cmd_batch_free(cmd_batch);
        send_policy_command();

    }
    if (rc == -1) {
      ATLAS_LOGGER_ERROR("Socket read error");
    }

} 

static void 
bind_socket(){
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        ATLAS_LOGGER_ERROR("Socket error");
    }

    int rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    while (rc == -1){
        sleep(1);
        ATLAS_LOGGER_ERROR("Socket bind error");
        rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    }
}

static void 
listen_socket(){
    int rc = listen(fd, 5);
    while(rc == -1)
    {
        ATLAS_LOGGER_ERROR("Socket listen error");
        rc = listen(fd, 5);
    }
}

int 
atlas_receive_commands_start()
{

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (*socket_path == '\0') {
        *addr.sun_path = '\0';
        strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
    } 
    else {
        strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
        unlink(socket_path);
    }

    bind_socket();

    listen_socket();

    atlas_sched_add_entry(fd, atlas_data_plane_callback);

    return fd;
}


