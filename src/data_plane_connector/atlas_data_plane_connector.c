#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../scheduler/atlas_scheduler.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../utils/atlas_config.h"
#include "../coap/atlas_coap_client.h"
#include "../coap/atlas_coap_response.h"
#include "../alarm/atlas_alarm.h"
#include "../utils/atlas_utils.h"
#include "../identity/atlas_identity.h"
#include "atlas_data_plane_connector.h"


#define ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN (2048)
#define ATLAS_CLIENT_POLICY_TIMEOUT_MS  (5000)
#define ATLAS_CLIENT_FEATURE_TIMEOUT_MS  (5000)
#define ATLAS_CLIENT_POLICY_COAP_PATH   "gateway/policy"
#define ATLAS_CLIENT_FEATURE_COAP_PATH   "gateway/reputation/feature"

static struct sockaddr_un addr;
static int fd = -1;
static int cl = -1;

static char *username;
static char *clientid;
static uint16_t policy_qos;
static uint16_t policy_packets_per_min;
static uint16_t policy_packets_maxlen;
static uint16_t packets_per_min;
static uint16_t packets_avg;

static void set_username(const uint8_t *user, uint16_t length)
{
    if (username)
        free(username);

    username = (char *) malloc(length + 1);
    memcpy(username, user, length);
    username[length] = 0;
}

static void set_clientid(const uint8_t *id, uint16_t length)
{
    if (clientid)
        free(clientid);

    clientid = (char *) malloc(length + 1);
    memcpy(clientid, id, length);
    clientid[length] = 0;
}

static void set_policy_qos(const uint8_t* qos)
{
    memcpy(&policy_qos, qos, sizeof(policy_qos));
}

static void set_policy_packets_per_min(const uint8_t* ppm)
{
    memcpy(&policy_packets_per_min, ppm, sizeof(policy_packets_per_min));
}

static void set_policy_packets_maxlen(const uint8_t* pack_maxlen)
{
    memcpy(&policy_packets_maxlen, pack_maxlen, sizeof(policy_packets_maxlen));
}

static void set_packets_per_min(const uint8_t* ppm){
    memcpy(&packets_per_min, ppm, sizeof(packets_per_min));
}

static void set_packets_avg(const uint8_t* pack_avg){
    memcpy(&packets_avg, pack_avg, sizeof(packets_avg));
}

uint16_t get_packets_per_min(){
    return packets_per_min;
}

uint16_t get_packets_avg(){
    return packets_avg;
}

static void
policy_alarm_callback()
{
    ATLAS_LOGGER_INFO("Policy alarm callback");
    send_policy_command();
}

static void 
policy_callback(const char *uri, atlas_coap_response_t resp_status,
         const uint8_t *resp_payload, size_t resp_payload_len)
{
    ATLAS_LOGGER_DEBUG("Policy callback executed");

    if (resp_status != ATLAS_COAP_RESP_OK) {
        ATLAS_LOGGER_ERROR("Error in sending the policy values");
        
        /* Start sending policy timer */
        if (atlas_alarm_set(ATLAS_CLIENT_POLICY_TIMEOUT_MS, policy_alarm_callback, ATLAS_ALARM_RUN_ONCE) < 0)
            ATLAS_LOGGER_ERROR("Error in scheduling a sending policy alarm!");
        
        return;
    }

    ATLAS_LOGGER_INFO("Sending policy to gateway is COMPLETED!");
}

void
send_policy_command()
{
    atlas_cmd_batch_t *cmd_batch;
    atlas_status_t status;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    uint16_t tmp;

    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    if (!clientid) {
        ATLAS_LOGGER_DEBUG("Clientid value is null, thus the firewall policy cannot be sent to gateway");
        return;
    }

    cmd_batch = atlas_cmd_batch_new();

    /* Add MQTT clientID */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_CLIENTID, strlen(clientid), (uint8_t*) clientid);
    
     /* Add policy qos value */
    tmp = htons(policy_qos);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_QOS,
                        sizeof(policy_qos), (uint8_t *) &tmp);

    /* Add policy packets per minute value */
    tmp = htons(policy_packets_per_min);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE,
                        sizeof(policy_packets_per_min), (uint8_t *) &tmp);

    /* Add policy packets maxlen value */
    tmp = htons(policy_packets_maxlen);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_MAXLEN,
                        sizeof(policy_packets_maxlen), (uint8_t *) &tmp);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    atlas_cfg_coap_get_uri(ATLAS_CLIENT_POLICY_COAP_PATH, uri);
    status = atlas_coap_client_request(uri, ATLAS_COAP_METHOD_PUT,
                                       cmd_buf, cmd_len, ATLAS_CLIENT_POLICY_TIMEOUT_MS,
                                       policy_callback);


    if (status != ATLAS_OK)
        ATLAS_LOGGER_ERROR("Error when sending policy request");

    atlas_cmd_batch_free(cmd_batch);
}

static atlas_status_t
atlas_reputation_resp_parse(const uint8_t *buf, uint16_t buf_len)
{
    atlas_cmd_batch_t *cmd_batch, *cmd_batch_send;
    const atlas_cmd_t *cmd;
    atlas_status_t status = ATLAS_OK;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;

    if (!buf || !buf_len)
        return ATLAS_INVALID_INPUT;

    cmd_batch = atlas_cmd_batch_new();
    
    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted feature reputation command");
        status = ATLAS_CORRUPTED_COMMAND;
        goto EXIT;
    }
    
    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE && cmd->length == sizeof(uint16_t)) {
            cmd_batch_send = atlas_cmd_batch_new();
            /* Add feature reputation value */
            atlas_cmd_batch_add(cmd_batch_send, ATLAS_CMD_DATA_PLANE_FEATURE, cmd->length,
                                cmd->value);
            atlas_cmd_batch_get_buf(cmd_batch_send, &cmd_buf, &cmd_len);

            /* Send reputation value to data plane */            
            if (write(cl, cmd_buf, cmd_len) != cmd_len)
                ATLAS_LOGGER_ERROR("Error writing to socket the reputation value.");
            
            atlas_cmd_batch_free(cmd_batch_send);
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }
     
EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}

static void 
feature_reputation_callback(const char *uri, atlas_coap_response_t resp_status,
                            const uint8_t *resp_payload, size_t resp_payload_len)
{
    ATLAS_LOGGER_DEBUG("Feature reputation callback executed");
    printf("Feature callback executed\n");

    if (resp_status != ATLAS_COAP_RESP_OK) {
        ATLAS_LOGGER_ERROR("Error in sending the feature request");
        printf("Error in sending the feature request\n");
        /* TODO: trimite eroare la data plane */
        return;
    }

    atlas_reputation_resp_parse(resp_payload, resp_payload_len);
    
    ATLAS_LOGGER_INFO("Sending feature reputation to data plane is COMPLETED!");
    printf("Sending feature reputation to data plane is COMPLETED!\n");
}

static void
atlas_feature_reputation_handle(const uint8_t *feature, uint16_t length)
{
    atlas_cmd_batch_t *cmd_batch;
    atlas_status_t status;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const char *identity = atlas_identity_get();
    char uri[ATLAS_URI_MAX_LEN] = { 0 };

    if (!feature || !length) {
        ATLAS_LOGGER_ERROR("Invalid feature reputation request");
        return;
    }

    cmd_batch = atlas_cmd_batch_new();

    /* Add identity */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_IDENTITY, strlen(identity), (uint8_t *)identity);
    
    /* Add feature */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_FEATURE, length, feature);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);

    atlas_cfg_coap_get_uri(ATLAS_CLIENT_FEATURE_COAP_PATH, uri);
    status = atlas_coap_client_request(uri, ATLAS_COAP_METHOD_PUT,
                                       cmd_buf, cmd_len, ATLAS_CLIENT_FEATURE_TIMEOUT_MS,
                                       feature_reputation_callback);

    if (status != ATLAS_OK){
        ATLAS_LOGGER_ERROR("Error when sending feature request");
        printf("Error when sending feature request\n");
    }

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_data_plane_parse_policy(const uint8_t *buf, uint16_t buf_len)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    atlas_status_t status;

    if (!buf || !buf_len) {
        ATLAS_LOGGER_ERROR("Corrupted policy command from data plane");
        return;	
    }

    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted policy command from data plane");
        atlas_cmd_batch_free(cmd_batch);
        return;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_USERNAME) {
            set_username(cmd->value, cmd->length);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_CLIENTID) {
            set_clientid(cmd->value, cmd->length);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_QOS ) {
            set_policy_qos(cmd->value);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE ) {
            set_policy_packets_per_min(cmd->value);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_MAXLEN ) {
            set_policy_packets_maxlen(cmd->value);
	}

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }

    atlas_cmd_batch_free(cmd_batch);

    send_policy_command();
}

static void
atlas_data_plane_read_cb(int fd)
{
    uint8_t buf[ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN];
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    atlas_status_t status;
    int rc;

    rc = read(cl, buf, sizeof(buf));
    if ( rc <= 0) {
        ATLAS_LOGGER_ERROR("Socket read error");
        return;
    }
    
    cmd_batch = atlas_cmd_batch_new();
    
    status = atlas_cmd_batch_set_raw(cmd_batch, buf, rc);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted command from data plane");
        atlas_cmd_batch_free(cmd_batch);
        return;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY) {
            atlas_data_plane_parse_policy(cmd->value, cmd->length);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE ) {
            set_packets_per_min(cmd->value);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_PACKETS_AVG ) {
            set_packets_avg(cmd->value);
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE){
            printf("request feature reputation command: %s\n", cmd->value);
            atlas_feature_reputation_handle(cmd->value, cmd->length);
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }

    atlas_cmd_batch_free(cmd_batch);
}

static void
atlas_data_plane_accept_cb(int fd)
{ 
    /* Allow only one connection */
    if (cl != -1) {
        ATLAS_LOGGER_DEBUG("Close existing unix socket...");
        atlas_sched_del_entry(cl);	
	close(cl);
    }

    cl = accept(fd, NULL, NULL);
    if (cl == -1) {
        ATLAS_LOGGER_ERROR("Socket accept error");
	return;
    }

    /* FIXME what if a connection already exists */
    atlas_sched_add_entry(cl, atlas_data_plane_read_cb);
} 

static atlas_status_t
bind_socket()
{
    int rc;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd  == -1) {
        ATLAS_LOGGER_ERROR("Socket error");
        return ATLAS_SOCKET_ERROR;
    }

    rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1) {
        ATLAS_LOGGER_ERROR("Socket bind error");
        return ATLAS_SOCKET_ERROR;
    }

    return ATLAS_OK;
}

static atlas_status_t
listen_socket()
{
    int rc;
    
    rc = listen(fd, 1);
    if (rc == -1) {
        ATLAS_LOGGER_ERROR("Socket listen error");
        return ATLAS_SOCKET_ERROR;
    }

    return ATLAS_OK;
}

atlas_status_t
atlas_data_plane_connector_start()
{
    atlas_status_t status;
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ATLAS_DATA_PLANE_UNIX_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    unlink(addr.sun_path);

    status = bind_socket();
    if (status != ATLAS_OK)
        return status;

    status = listen_socket();
    if (status != ATLAS_OK)
        return ATLAS_OK;

    atlas_sched_add_entry(fd, atlas_data_plane_accept_cb);

    return ATLAS_OK;
}

