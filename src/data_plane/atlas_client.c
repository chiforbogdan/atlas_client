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
#include "MQTTClient.h"

#define SLEEPTIME 5

char *socket_path = "\0hidden";
int fd;
struct sockaddr_un addr;
pthread_mutex_t mutex;
pthread_t init_t;
int payload_samples = 0, payload_total = 0, payload_avg = 0;


struct registration{
    char* username;
    char* clientid;
    uint16_t policy;
}client;


static void *register_to_atlas_client();
static void send_username_command();
static void send_clientid_command();
static void send_policy_command();
static void send_packets_per_minute_command();
static void send_packets_avg_command();
static void write_to_socket(uint8_t* buffer);
static void restore_payload();

static void *register_to_atlas_client(){
    
    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");
    
    int rc;

    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	ATLAS_LOGGER_ERROR("DP: Socket error");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (*socket_path == '\0') {
	*addr.sun_path = '\0';
	strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
    } 
    else {
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    }
    rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    while(rc == -1){
	sleep(1);
	ATLAS_LOGGER_ERROR("DP: Connect error");
	rc = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    }

    send_username_command(client.username);
    sleep(1);

    send_clientid_command(client.clientid);
    sleep(1);

    send_policy_command(client.policy);
    sleep(1);
    
    while(1){
	send_packets_per_minute_command(payload_samples);
	sleep(1);

	send_packets_avg_command(payload_avg);
	sleep(1);
	
	restore_payload();
	sleep(SLEEPTIME);
    }
}

static void send_username_command(){
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_USERNAME, strlen(client.username), (uint8_t *)client.username);
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf);
    
    atlas_cmd_batch_free(cmd_batch);
}

static void send_clientid_command(){
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_CLIENTID, strlen(client.clientid), (uint8_t *)client.clientid);
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf);
    
    atlas_cmd_batch_free(cmd_batch);
}

static void send_policy_command(){
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY, sizeof(client.policy), (uint8_t *)&client.policy);
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf);
    
    atlas_cmd_batch_free(cmd_batch);
}

static void send_packets_per_minute_command(){
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE, sizeof(payload_samples), (uint8_t *)&payload_samples);
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf);
    
    atlas_cmd_batch_free(cmd_batch);
}

static void send_packets_avg_command(){
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_AVG, sizeof(payload_avg), (uint8_t *)&payload_avg);
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf);
    
    atlas_cmd_batch_free(cmd_batch);
}

static void write_to_socket(uint8_t* buffer){
    int n = -1;
    while(n<0){
	n = write(fd, (char*)&buffer, sizeof(buffer));   
	if (n < 0){
	     ATLAS_LOGGER_ERROR("DP: ERROR writing to socket.");  
	     close(fd);
	     sleep(2);
	     ATLAS_LOGGER_DEBUG("DP: Socket reconnecting...");
	     fd = socket(AF_UNIX, SOCK_STREAM, 0);
	     if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		ATLAS_LOGGER_ERROR("DP: Connect error");
	     }
	     n = write(fd, (char*)&buffer, sizeof(buffer));   
	 }
     }
}	

static void restore_payload(){

    pthread_mutex_lock(&mutex); 

    payload_samples = payload_total = payload_avg = 0;

    pthread_mutex_unlock(&mutex);
    
}

void atlas_pkt_received(int payload)
{

    pthread_mutex_lock(&mutex);
    
    payload_samples++;
    payload_total+=payload;
    payload_avg = payload_total/payload_samples;

    pthread_mutex_unlock(&mutex);
}

void atlas_init( char* user, char* client_id, uint16_t pol)
{
    client.username = strdup(user);
    client.clientid = strdup(client_id);
    client.policy = pol; 

    pthread_create(&init_t, NULL, &register_to_atlas_client, NULL);
}

