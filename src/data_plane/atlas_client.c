#include <stdio.h>
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

#define SLEEPTIME 10

int fd;
struct sockaddr_un addr;
pthread_mutex_t mutex;
pthread_t init_t;
int payload_samples = 0, payload_total = 0, payload_avg = 0;


struct registration{
    char* username;
    char* clientid;
    uint16_t qos;
    uint16_t packets_per_min;
    uint16_t packets_maxlen;
}client;


static void *register_to_atlas_client();
static void send_registration_command();
static void send_statistics_command();
static void write_to_socket(const uint8_t* buffer,uint16_t cmd_len);
static void socket_connect();
static void restore_payload();
static void send_reputation_command(char *feature);

static void 
*register_to_atlas_client(){
    
    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, ATLAS_DATA_PLANE_UNIX_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    socket_connect();
    
    send_registration_command();
   
    
    while(1){
        
        send_statistics_command();
	
        restore_payload();

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
    write_to_socket(cmd_buf_outer, cmd_outer_len);
    
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
    
    /* Add packets per minute received*/
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE, sizeof(payload_samples), (uint8_t *)&payload_samples);
    
    /* Add average length of received packets */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_AVG, sizeof(payload_avg), (uint8_t *)&payload_avg);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf, cmd_len);
    
    atlas_cmd_batch_free(cmd_batch);
}




static void 
socket_connect()
{

    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        ATLAS_LOGGER_ERROR("DP: Socket error");
    }

    int rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    while(rc == -1){
	sleep(1);
        ATLAS_LOGGER_ERROR("DP: Connect error");
        rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    }
    ATLAS_LOGGER_DEBUG("DP: Socket connected");
}

static void 
write_to_socket(const uint8_t* cmd_buf, uint16_t cmd_len)
{
    int n = write(fd, cmd_buf, cmd_len); 
    
    while(n<0){
	     ATLAS_LOGGER_ERROR("DP: ERROR writing to socket."); 
	     close(fd);
	     sleep(2);
	     
	     socket_connect();
	     
	     n = write(fd, cmd_buf, cmd_len);  
     }
     
     
}	

static void 
restore_payload()
{

    pthread_mutex_lock(&mutex); 

    payload_samples = payload_total = payload_avg = 0;

    pthread_mutex_unlock(&mutex);
    
}

void 
atlas_pkt_received(int payload)
{

    pthread_mutex_lock(&mutex);
    
    payload_samples++;
    payload_total+=payload;
    payload_avg = payload_total/payload_samples;

    pthread_mutex_unlock(&mutex);
}

void
atlas_reputation_request(char *feature)
{
    send_reputation_command(feature);
}

static void 
send_reputation_command(char *feature)
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    uint8_t buf[512];
    
    cmd_batch = atlas_cmd_batch_new();
    
     /* Add feature */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_FEATURE, strlen(feature),
                        (uint8_t *)feature);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    printf("REQUEST REPUTATION\n");
    write_to_socket(cmd_buf, cmd_len);
    printf("\n");
    
    int r = read(fd, buf, sizeof(buf)); 
    printf("Am primit: %s  %d\n", buf, fd);
    while( r <= 0 ){
        printf("Read error\n");
        r = read(fd, buf, sizeof(buf));
    } 
    
    atlas_cmd_batch_free(cmd_batch);
}

void 
atlas_init( char* user, char* client_id, uint16_t qos, uint16_t ppm, uint16_t pack_maxlen)
{
    client.username = strdup(user);
    client.clientid = strdup(client_id);
    client.qos = qos;
    client.packets_per_min = ppm; 
    client.packets_maxlen = pack_maxlen; 

    pthread_create(&init_t, NULL, &register_to_atlas_client, NULL);
}
