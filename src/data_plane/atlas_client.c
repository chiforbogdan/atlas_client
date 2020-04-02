#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../utils/atlas_utils.h"
#include "MQTTClient.h"
#include "atlas_client.h"

#define SLEEPTIME (60)
#define ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN (2048)
#define ATLAS_CLIENT_DATA_PLANE_SOCKET_READ_TIMEOUT_SEC (5)
#define ATLAS_CLIENT_DATA_PLANE_FEEDBACKS_SCORE_MAX (100)

typedef struct client_info
{
    /* Firewall QoS value */
    uint16_t qos;
    /* Firewall packets per minute value */
    uint16_t packets_per_min;
    /* Firewall packet max length value */
    uint16_t packets_maxlen;
} client_info_t;

static volatile int fd = -1;
static struct sockaddr_un addr;
static pthread_mutex_t mutex;
static pthread_t init_t;
static int payload_samples;
static int payload_total;
static int payload_avg;
static client_info_t client;

static void *register_to_atlas_client();
static void send_registration_command();
static void send_statistics_command();
static int write_to_socket(const uint8_t* buffer,uint16_t cmd_len);
static void write_to_socket_retry(const uint8_t* buffer,uint16_t cmd_len);
static void socket_connect();

static void*
register_to_atlas_client()
{
    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");
    
    while(1) {
        
        send_statistics_command();

        sleep(SLEEPTIME);
    }

    return NULL;
}

static atlas_status_t
send_identity_command(char **identity)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t buf[ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN];
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const atlas_cmd_t *cmd;
    int bytes;
    atlas_status_t status = ATLAS_OK;
     
    /* Create identity payload*/
    cmd_batch = atlas_cmd_batch_new();
    
    /* Add client_id */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_GET_IDENTITY, 0, NULL);
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    
    /* Send data to atlas_client */
    write_to_socket_retry(cmd_buf, cmd_len);
    
    atlas_cmd_batch_free(cmd_batch);
    cmd_batch = NULL;

    /* Read command from client */
    bytes = read(fd, buf, sizeof(buf));
    if (bytes <= 0) {
        ATLAS_LOGGER_ERROR("Error when reading identity");
        status = ATLAS_SOCKET_ERROR;
        goto EXIT;
    }

    cmd_batch = atlas_cmd_batch_new();
    
    status = atlas_cmd_batch_set_raw(cmd_batch, buf, bytes);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted command from atlas_client");
        goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    *identity = NULL;
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_IDENTITY) {
            *identity = (char *) calloc(1, cmd->length + 1);
            memcpy(*identity, cmd->value, cmd->length);
            goto EXIT;
        }
        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }

EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}

static void 
send_registration_command()
{
    atlas_cmd_batch_t *cmd_batch_inner;
    atlas_cmd_batch_t *cmd_batch_outer;
    uint8_t *cmd_buf_inner = NULL;
    uint16_t cmd_inner_len = 0;
    uint8_t *cmd_buf_outer = NULL;
    uint16_t cmd_outer_len = 0;
    
    /* Create policy payload*/
    cmd_batch_inner = atlas_cmd_batch_new();
    
    /* Add policy qos */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_POLICY_QOS, 
                        sizeof(client.qos), (uint8_t *)&client.qos);
    
    /* Add policy packets per minute */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE,
                        sizeof(client.packets_per_min), (uint8_t *)&client.packets_per_min);
    
    /* Add policy packets average length*/
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_MAXLEN,
                        sizeof(client.packets_maxlen), (uint8_t *)&client.packets_maxlen);
    
    atlas_cmd_batch_get_buf(cmd_batch_inner, &cmd_buf_inner, &cmd_inner_len);
    
    cmd_batch_outer = atlas_cmd_batch_new();

    /* Add inner command: username, client id, qos, packets per minute, packets max length */
    atlas_cmd_batch_add(cmd_batch_outer, ATLAS_CMD_DATA_PLANE_POLICY, cmd_inner_len, cmd_buf_inner);
    atlas_cmd_batch_get_buf(cmd_batch_outer, &cmd_buf_outer, &cmd_outer_len);
    
    /* Send data to atlas_client */
    write_to_socket_retry(cmd_buf_outer, cmd_outer_len);
    
    atlas_cmd_batch_free(cmd_batch_inner);
    atlas_cmd_batch_free(cmd_batch_outer);
}

static void 
send_statistics_command()
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();

    pthread_mutex_lock(&mutex);
   
    /* Add packets per minute received*/
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE, sizeof(payload_samples), (uint8_t *)&payload_samples);
    
    /* Add average length of received packets */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_AVG, sizeof(payload_avg), (uint8_t *)&payload_avg);

    /* Reset statistics */
    payload_samples = payload_total = payload_avg = 0;
    
    pthread_mutex_unlock(&mutex);

    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket_retry(cmd_buf, cmd_len);
    
    atlas_cmd_batch_free(cmd_batch);
}

static void 
socket_connect()
{
    int rc = -1;
    struct timeval timeout;
    timeout.tv_sec = ATLAS_CLIENT_DATA_PLANE_SOCKET_READ_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    /* Init ATLAS client socket */
    close(fd);
    if ((fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
        ATLAS_LOGGER_ERROR("DP: Socket error");
        return;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    while(rc) {
        rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (!rc) {
            break;
        } else
            ATLAS_LOGGER_ERROR("DP: Connect error");
	
        sleep(1);
    }

    ATLAS_LOGGER_DEBUG("DP: Socket connected");

    send_registration_command();
}

static void
write_to_socket_retry(const uint8_t* cmd_buf, uint16_t cmd_len)
{
    int bytes;

    bytes = write_to_socket(cmd_buf, cmd_len);
    while(bytes != cmd_len) {
        ATLAS_LOGGER_ERROR("DP: ERROR writing to socket.");

        socket_connect();
     
        bytes = write_to_socket(cmd_buf, cmd_len);
    }
}	

static int
write_to_socket(const uint8_t* cmd_buf, uint16_t cmd_len)
{
    return write(fd, cmd_buf, cmd_len); 
}	

void 
atlas_pkt_received(int payload_len)
{
    pthread_mutex_lock(&mutex);
    
    payload_samples++;
    payload_total += payload_len;
    payload_avg = payload_total / payload_samples;

    pthread_mutex_unlock(&mutex);
}

static atlas_status_t
send_feedback_command(atlas_feedback_t *feedback)
{
    atlas_cmd_batch_t *cmd_batch_inner;
    atlas_cmd_batch_t *cmd_batch_outer;
    uint8_t *cmd_buf_inner = NULL;
    uint16_t cmd_inner_len = 0;
    uint8_t *cmd_buf_outer = NULL;
    uint16_t cmd_outer_len = 0;
    int bytes;
    uint16_t tmp;
    atlas_status_t status = ATLAS_OK;

    /* Create feedback payload */
    cmd_batch_inner = atlas_cmd_batch_new();

    /* Add identity */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_IDENTITY, strlen(feedback->identity),
                        (uint8_t *)feedback->identity);

    /* Add sensor type */
    tmp = htons(feedback->sensor_type);
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_SENSOR_TYPE, sizeof(tmp),
                        (uint8_t *)&tmp);
                        
    /* Add sensor feedback value */
    tmp = htons(feedback->sensor_score);
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_SENSOR, sizeof(tmp),
                        (uint8_t *)&tmp);

    /* Add response time feedback value */
    tmp = htons(feedback->time_score);
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_RESPONSE_TIME, sizeof(tmp), 
                        (uint8_t *)&tmp);

    atlas_cmd_batch_get_buf(cmd_batch_inner, &cmd_buf_inner, &cmd_inner_len);

    cmd_batch_outer = atlas_cmd_batch_new();

    /* Add inner command: identity, sensor feedback score, response time feedback score  */
    atlas_cmd_batch_add(cmd_batch_outer, ATLAS_CMD_DATA_PLANE_FEEDBACK, cmd_inner_len, cmd_buf_inner);
    atlas_cmd_batch_get_buf(cmd_batch_outer, &cmd_buf_outer, &cmd_outer_len);
    
    /* Send data to atlas_client */
    bytes = write_to_socket(cmd_buf_outer, cmd_outer_len);
    if (bytes != cmd_outer_len) {
        ATLAS_LOGGER_ERROR("Error when sending feedback to atlas client");
        status = ATLAS_GENERAL_ERR;
    }

    atlas_cmd_batch_free(cmd_batch_inner);
    atlas_cmd_batch_free(cmd_batch_outer);

    return status;
}

atlas_status_t
atlas_reputation_feedback(atlas_feedback_t *feedback, size_t feedback_len)
{
    int i;

    if (!feedback || !feedback_len)
        return ATLAS_INVALID_INPUT;   
   
    /* Validate feedback */
    for (i = 0; i < feedback_len; i++) {
        if (feedback[i].sensor_score > ATLAS_CLIENT_DATA_PLANE_FEEDBACKS_SCORE_MAX)
            return ATLAS_INVALID_INPUT;
        if (feedback[i].time_score > ATLAS_CLIENT_DATA_PLANE_FEEDBACKS_SCORE_MAX)
            return ATLAS_INVALID_INPUT;
        if (!feedback[i].identity)
            return ATLAS_INVALID_INPUT;

        /* Send feedback score */
        send_feedback_command(feedback + i);
   }
    
    return ATLAS_OK;
}

atlas_status_t 
atlas_reputation_request(atlas_sensor_t sensor_type, char **identity)
{
    atlas_cmd_batch_t *cmd_batch = NULL;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const atlas_cmd_t *cmd;
    uint8_t buf[ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN];
    uint16_t tmp;
    atlas_status_t status = ATLAS_OK;
    int bytes;

    if (!identity)
        return ATLAS_INVALID_INPUT; 
   
    cmd_batch = atlas_cmd_batch_new();
    
     /* Add feature */
    tmp = htons(sensor_type);
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_FEATURE_REPUTATION, sizeof(tmp),
                        (uint8_t *)&tmp);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    sprintf((char*)buf, "Request identity with best reputation for sensor type %d", sensor_type);
    ATLAS_LOGGER_DEBUG((char*)buf);
    
    bytes = write_to_socket(cmd_buf, cmd_len);
    if (bytes != cmd_len) {
        ATLAS_LOGGER_ERROR("Error when writing reputation request to client");
        status = ATLAS_SOCKET_ERROR;
        goto EXIT;
    }

    atlas_cmd_batch_free(cmd_batch);
    cmd_batch = NULL;
   
    bytes = read(fd, buf, sizeof(buf));
    if (bytes <= 0) {
        ATLAS_LOGGER_ERROR("Error when reading the reputation value for requested feature");
        status = ATLAS_SOCKET_ERROR;
        goto EXIT;
    }

    cmd_batch = atlas_cmd_batch_new();
    
    status = atlas_cmd_batch_set_raw(cmd_batch, buf, bytes);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted command from atlas_client");
        goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE_REPUTATION) {            
            *identity = (char *) calloc(1, cmd->length + 1);
            memcpy(*identity, cmd->value, cmd->length); 
            goto EXIT;
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE_ERROR) {
            ATLAS_LOGGER_ERROR("No identity with best reputation");
            status = ATLAS_GENERAL_ERR;
            goto EXIT;
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }

EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}

atlas_status_t
atlas_init(uint16_t qos, uint16_t ppm, uint16_t pack_maxlen, char **identity)
{
    atlas_status_t status;

    if (!identity)
        return ATLAS_INVALID_INPUT;

    /* Set application data */
    client.qos = qos;
    client.packets_per_min = ppm; 
    client.packets_maxlen = pack_maxlen; 

    /* Init mutex */
    pthread_mutex_init(&mutex, NULL);

    /* Set socket info */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ATLAS_DATA_PLANE_UNIX_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    /* Connect to atlas client */
    socket_connect();

    /* Get identity from atlas client */
    status = send_identity_command(identity);
    if (status != ATLAS_OK)
        return status;

    /* Create ATLAS background client communication thread*/
    pthread_create(&init_t, NULL, &register_to_atlas_client, NULL);

    return ATLAS_OK;
}
