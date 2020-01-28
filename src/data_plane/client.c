#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>
#include <pcap.h> 
#include <errno.h> 
#include <arpa/inet.h> 
#include <netinet/if_ether.h>
#include <unistd.h>
#include <pthread.h>

#include "../logger/atlas_logger.h"
#include "MQTTClient.h"

#define SLEEPTIME 5
#define IP "host 192.168.1.103"
#define ADDRESS "tcp://127.0.0.1:1883"
#define CLIENTID "clientID"
#define TOPICMAX "MAX"
#define TOPIC "TOPIC"
#define PAYLOAD "ALERT"
#define THRESHOLD_PACKETS_PER_SECOND 5
#define THRESHOLD_NO_OF_PACKETS 25
#define QOS 1
#define TIMEOUT 1000L

int  fd;
struct sockaddr_un addr;
char *socket_path = "\0hidden";
volatile MQTTClient_deliveryToken deliveredtoken;
volatile MQTTClient clientMQTT;

struct registration{
    char* username;
    char* clientid;
    char* policy;
};

void *register_to_atlas_client(void *client){
    
    ATLAS_LOGGER_DEBUG("DP: Register to atlas_client");
    
    struct registration *client_r =(struct registration*) client;
    int rc;
    
    char buffer[256];
    sprintf(buffer, "%s|%s|%s", client_r->username, 
		client_r->clientid, client_r->policy);
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
    write_to_socket(buffer);
    sleep(1);
    
    while(1){
	send_values_to_atlas_client();
	sleep(SLEEPTIME);
    }


}

void atlas_init(pthread_t init_t, char* user, int client_id, char* pol){
    
    
    struct registration client;
    client.username = strdup(user);
    client.clientid = strdup(client_id);
    client.policy = strdup(pol); 
    
    pthread_create(&init_t, NULL, &register_to_atlas_client, (void*)&client);
    
    
}


void write_to_socket(char*buffer){
    int n = -1;
    while(n<0){
	n = write(fd, buffer, strlen(buffer));   
	if (n < 0){
	     ATLAS_LOGGER_ERROR("DP: ERROR writing to socket.");  
	     close(fd);
	     sleep(2);
	     ATLAS_LOGGER_DEBUG("DP: Socket reconnecting...");
	     fd = socket(AF_UNIX, SOCK_STREAM, 0);
	     if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		ATLAS_LOGGER_ERROR("DP: Connect error");
	     }
	     n = write(fd, buffer, strlen(buffer));   
	 }
     }
}

void write_to_file(char *file, char *type, int RX){
	FILE * f = fopen(file, type);	
	fprintf(f, "%d\n", RX);
	fclose(f);
}	

void send_values_to_atlas_client(){
	ATLAS_LOGGER_DEBUG("DP: Send average packet length.");
		FILE *f ;
		if ((f = fopen("MQTT_payload_length.txt", "r")) == NULL){
			ATLAS_LOGGER_ERROR("DP: No packets received");
		}
		else{
			int data=0;
			int i = 0, pacLen;
			while(!feof(f)){
			    fscanf(f, "%d", &pacLen);
			    data=data+pacLen;
			    i++;
			}
			data=data/i;
			char buff[256];
			sprintf(buff, "averageLength %d", data);
			write_to_socket(buff);
			sleep(0.1);
			sprintf(buff, "pkt/min %d", i);
			write_to_socket(buff);
			fclose(f);
		}		
}

void publish(MQTTClient client, MQTTClient_message pubmsg, 
	    MQTTClient_deliveryToken token, int rc, char *topic){
	pubmsg.payload = PAYLOAD;
    	pubmsg.payloadlen = strlen(PAYLOAD);
    	pubmsg.qos = QOS;
    	pubmsg.retained = 0;
	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
	rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
	char buff[256];
	sprintf(buff, "DP: Message %s with delivery token %d delivered.", PAYLOAD, token);
    	ATLAS_LOGGER_DEBUG(buff);

}

void subscribe(MQTTClient client, char* topic){
	MQTTClient_subscribe(client, topic, QOS);
	char buff[256];
	sprintf(buff, "Subscribing to topic %s for client %s using QOS %d\n", topic, CLIENTID, QOS);
	ATLAS_LOGGER_DEBUG(buff);
 	
}

void connlost(void *context, char *cause){
    char buff[256];
    sprintf(buff, "Connection lost, cause: %s.", cause);
    ATLAS_LOGGER_DEBUG(buff);
}

void delivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


int msgarrvd(void *context, char *topicName, int topicLen, 
	    MQTTClient_message *message){
    int i;
    char* payloadptr;

    write_to_file("MQTT_payload_length.txt", "a", message->payloadlen);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

MQTTClient start_MQTTclient(){
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token = 0;
    int rc;

    if(MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) 
		!= MQTTCLIENT_SUCCESS){
        ATLAS_LOGGER_ERROR("Failed to create MQTTclient.");
        //exit(-1);
    }
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        ATLAS_LOGGER_ERROR("Failed to connectMQTTclient");
        //exit(-1);
    }
      
    MQTTClient_subscribe(client, TOPIC, QOS);
    printf("Subscribing to topic %s for client %s using QOS %d\n", TOPIC, CLIENTID, QOS);
       
    return client;
}


