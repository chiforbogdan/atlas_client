#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "../utils/atlas_utils.h"
#include "MQTTClient.h"
#include "atlas_data_plane.h"

#define SLEEPTIME (60)
#define ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN (2048)
#define ATLAS_CLIENT_DATA_PLANE_FEEDBACK_WINDOW_SIZE (10)

static volatile int fd = -1;
static struct sockaddr_un addr;
static pthread_mutex_t mutex;
static pthread_t init_t;
static int payload_samples = 0;
static int payload_total = 0;
static int payload_avg = 0;
char* client_rep_id;

struct registration
{
    char* username;
    char* clientid;
    uint16_t qos;
    uint16_t packets_per_min;
    uint16_t packets_maxlen;
} client;

static void *register_to_atlas_client();
static void send_registration_command();
static void send_statistics_command();
static int write_to_socket(const uint8_t* buffer,uint16_t cmd_len);
static void write_to_socket_retry(const uint8_t* buffer,uint16_t cmd_len);
static void socket_connect();
atlas_status_t send_reputation_command(const char *feature);

static void set_client_rep_id(const uint8_t *client, uint16_t length)
{
    if (client_rep_id)
        free(client_rep_id);

    client_rep_id = (char *) malloc(length + 1);
    memcpy(client_rep_id, client, length);
    client_rep_id[length] = 0;
}

static void*
register_to_atlas_client()
{
    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ATLAS_DATA_PLANE_UNIX_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    socket_connect();
    
    while(1) {
        
        send_statistics_command();

        sleep(SLEEPTIME);
    }

    return NULL;
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
    
    /* Add username */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_POLICY_USERNAME, strlen(client.username),
                        (uint8_t *)client.username);
    
    /* Add client_id */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_POLICY_CLIENTID, strlen(client.clientid),
                        (uint8_t *)client.clientid);
                        
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
    static int cnt = 0;
    
    cmd_batch = atlas_cmd_batch_new();

    pthread_mutex_lock(&mutex);
   
    payload_samples = cnt++; 
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

    /* Init ATLAS client socket */
    close(fd);
    if ((fd = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
        ATLAS_LOGGER_ERROR("DP: Socket error");
        return;
    }

    printf("Connect again\n");

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
atlas_pkt_received(int payload)
{
    pthread_mutex_lock(&mutex);
    
    payload_samples++;
    payload_total += payload;
    payload_avg = payload_total / payload_samples;

    pthread_mutex_unlock(&mutex);
}

static uint16_t
compute_feedback(int tmp)
{
    return 1;
}

atlas_status_t
send_feedback_command(char* payload, uint16_t time_ms)
{
    atlas_cmd_batch_t *cmd_batch_inner;
    atlas_cmd_batch_t *cmd_batch_outer;
    uint8_t *cmd_buf_inner = NULL;
    uint16_t cmd_inner_len = 0;
    uint8_t *cmd_buf_outer = NULL;
    uint16_t cmd_outer_len = 0;
    const atlas_cmd_t *cmd;
    uint8_t buf[ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN];
    atlas_status_t status = ATLAS_OK;
    int bytes;
    char* clientID;
    int feature_value;
    uint16_t tmp;

    char *p = strtok(payload, ":");
    clientID = p;
    p = strtok(NULL, ":");
    feature_value = atoi (p);
    tmp = compute_feedback(feature_value);
    
    /* Create feedback payload*/
    cmd_batch_inner = atlas_cmd_batch_new();

    /* Add clientID */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_CLIENTID, strlen(clientID),
                        (uint8_t *)clientID);
    
    /* Add feature */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_FEATURE, strlen("temp"),
                        (uint8_t *)"temp");
                        
    /* Add value */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_VALUE, sizeof(tmp), (uint8_t *)&tmp);

    /* Add response time */
    atlas_cmd_batch_add(cmd_batch_inner, ATLAS_CMD_DATA_PLANE_FEEDBACK_RESPONSE_TIME, sizeof(time_ms), (uint8_t *)&time_ms);

    atlas_cmd_batch_get_buf(cmd_batch_inner, &cmd_buf_inner, &cmd_inner_len);

    cmd_batch_outer = atlas_cmd_batch_new();

    /* Add inner command: clientID, feature type, feedback value */
    atlas_cmd_batch_add(cmd_batch_outer, ATLAS_CMD_DATA_PLANE_FEEDBACK, cmd_inner_len, cmd_buf_inner);
    atlas_cmd_batch_get_buf(cmd_batch_outer, &cmd_buf_outer, &cmd_outer_len);
    
    /* Send data to atlas_client */
    write_to_socket_retry(cmd_buf_outer, cmd_outer_len);
    
    atlas_cmd_batch_free(cmd_batch_inner);
    atlas_cmd_batch_free(cmd_batch_outer);

    /* Read command from client */
    bytes = read(fd, buf, sizeof(buf));
    if (bytes <= 0) {
        ATLAS_LOGGER_ERROR("Error when reading confirmation of receiving feedback");
        status = ATLAS_SOCKET_ERROR;
        goto EXIT;
    }

    cmd_batch_inner = atlas_cmd_batch_new();
    
    status = atlas_cmd_batch_set_raw(cmd_batch_inner, buf, bytes);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted command from atlas_client");
        printf("Corrupted command from atlas_client\n");
        goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch_inner, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_FEEDBACK_ERROR) {
            ATLAS_LOGGER_ERROR("Error in sending the feedback to gateway");
            printf("Error in sending the feedback to gateway\n");
            status = ATLAS_GENERAL_ERR;
            goto EXIT;
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE_SUCCESSFULLY_DELIVERED) {
            ATLAS_LOGGER_DEBUG("Feedback successfully delivered to gateway");
            printf("Feedback successfully delivered to gateway\n");
            goto EXIT;
        }

        cmd = atlas_cmd_batch_get(cmd_batch_inner, cmd);
    }

EXIT:
    atlas_cmd_batch_free(cmd_batch_inner);

    return status;
}

atlas_status_t 
atlas_reputation_request(char *feature)
{
    return send_reputation_command(feature);
}

atlas_status_t 
send_reputation_command(const char *feature)
{
    atlas_cmd_batch_t *cmd_batch = NULL;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    const atlas_cmd_t *cmd;
    uint8_t buf[ATLAS_CLIENT_DATA_PLANE_BUFFER_LEN];
    atlas_status_t status = ATLAS_OK;
    int bytes;
    
    
    cmd_batch = atlas_cmd_batch_new();
    
     /* Add feature */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_FEATURE_REPUTATION, strlen(feature),
                        (uint8_t *)feature);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    printf("REQUEST REPUTATION\n");
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
        printf("Corrupted command from atlas_client\n");
        goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE_REPUTATION) {            
            set_client_rep_id(cmd->value, cmd->length);
            printf("Am primit de la GW: %s\n", client_rep_id);
            request_feature_values();
            goto EXIT;
        } else if (cmd->type == ATLAS_CMD_DATA_PLANE_FEATURE_ERROR) {
            ATLAS_LOGGER_ERROR("No reputation value received.");
            printf("Am primit de la GW: ERROR\n");
            status = ATLAS_GENERAL_ERR;
            goto EXIT;
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }

EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}

void 
atlas_init(const char* user, const char* client_id,
           uint16_t qos, uint16_t ppm, uint16_t pack_maxlen)
{
    /* Set application data */
    client.username = strdup(user);
    client.clientid = strdup(client_id);
    client.qos = qos;
    client.packets_per_min = ppm; 
    client.packets_maxlen = pack_maxlen; 

    /* Init mutexes */
    pthread_mutex_init(&mutex, NULL);

    /* Create ATLAS background client communication thread*/
    pthread_create(&init_t, NULL, &register_to_atlas_client, NULL);
}
