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

#define SLEEPTIME 60

char *socket_path = "\0hidden";
int fd;
struct sockaddr_un addr;
pthread_mutex_t mutex;
pthread_t init_t;
int payload_samples = 0, payload_total = 0, payload_avg = 0;


struct registration{
    char* username;
    char* clientid;
    uint16_t packets_per_min;
    uint16_t packets_avg;
}client;


static void *register_to_atlas_client();
static void send_registration_command();
static void send_statistics_command();
static void write_to_socket(uint8_t* buffer,uint16_t cmd_len);
static void socket_connect();
static void restore_payload();

static void *register_to_atlas_client(){
    
    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (*socket_path == '\0') {
	*addr.sun_path = '\0';
	strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
    } 
    else {
	strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    }

    socket_connect();
    
    //send_registration_command();
    sleep (2);
    
    while(1){
        
        send_statistics_command();
	
        restore_payload();

        sleep(SLEEPTIME);
    }
    return NULL;
}

static void send_registration_command()
{
    printf("send_registration_command");
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    
    /* Add username */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_USERNAME, strlen(client.username), (uint8_t *)client.username);
    
    /* Add client_id */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_CLIENTID, strlen(client.clientid), (uint8_t *)client.clientid);
    
    /* Add policy packets per minute */
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE, sizeof(client.packets_per_min), (uint8_t *)&client.packets_per_min);
    
    /* Add policy packets average length*/
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_AVG, sizeof(client.packets_avg), (uint8_t *)&client.packets_avg);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf, cmd_len);
    
    atlas_cmd_batch_free(cmd_batch);
    
}

static void send_statistics_command()
{
    atlas_cmd_batch_t *cmd_batch;
    uint8_t *cmd_buf = NULL;
    uint16_t cmd_len = 0;
    
    cmd_batch = atlas_cmd_batch_new();
    
    
    /* Add packets per minute received*/
    atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE, sizeof(payload_samples), (uint8_t *)&payload_samples);
    
    /* Add average length of received packets */
    //atlas_cmd_batch_add(cmd_batch, ATLAS_CMD_DATA_PLANE_PACKETS_AVG, sizeof(payload_avg), (uint8_t *)&payload_avg);
    
    atlas_cmd_batch_get_buf(cmd_batch, &cmd_buf, &cmd_len);
    write_to_socket(cmd_buf, cmd_len);
    
    atlas_cmd_batch_free(cmd_batch);
}




static void socket_connect(){
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

static void write_to_socket(uint8_t* cmd_buf, uint16_t cmd_len){
    int n = write(fd, (char*)&cmd_buf, cmd_len); 
    printf("\n%d\n ",n) ;  

    while(n<0){
	     ATLAS_LOGGER_ERROR("DP: ERROR writing to socket.");  
	     close(fd);
	     sleep(2);
	    
	     socket_connect();
	     
	     n = write(fd, (char*)&cmd_buf, cmd_len);  
         printf("%d\n ",n) ;
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

void atlas_init( char* user, char* client_id, uint16_t ppm, uint16_t pack_avg)
{
    client.username = strdup(user);
    client.clientid = strdup(client_id);
    client.packets_per_min = ppm; 
    client.packets_avg = pack_avg; 

    pthread_create(&init_t, NULL, &register_to_atlas_client, NULL);
}

