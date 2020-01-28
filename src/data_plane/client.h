#ifndef CLIENT_H
#define CLIENT_H


#include "MQTTClient.h"

void *register_to_atlas_client(void *client);

void atlas_init(pthread_t init_t, char* user, int client_id, char* pol);

void write_to_socket(char*buffer);

void write_to_file(char *file, char *type, int RX);

void send_values_to_atlas_client();

void publish(MQTTClient client, MQTTClient_message pubmsg, 
	    MQTTClient_deliveryToken token, int rc, char *topic);
	    
void subscribe(MQTTClient client, char* topic);

void connlost(void *context, char *cause);

void delivered(void *context, MQTTClient_deliveryToken dt);

int msgarrvd(void *context, char *topicName, int topicLen, 
	    MQTTClient_message *message);
	    
MQTTClient start_MQTTclient();


#endif

