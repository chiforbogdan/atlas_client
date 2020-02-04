#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "atlas_client.h"
#include "MQTTClient.h"
#include "../logger/atlas_logger.h"

#define ADDRESS "tcp://127.0.0.1:1883"
#define CLIENTID "client"
#define QOS 1
#define TIMEOUT 1000L

volatile MQTTClient_deliveryToken deliveredtoken;
volatile MQTTClient clientMQTT;
MQTTClient_deliveryToken token = 0;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient atlasMQTTclient;

typedef struct publish_struct{
    char* feature;
    int publish_rate;
    int base_value;
    int deviation;
}publish_struct_t;
    
static int random_number_generator(int base_value, int deviation){
    int lower, upper;
    lower = base_value - deviation;
    upper = base_value + deviation;
    
    return random() % (upper - lower +deviation) + lower;
}


void* publish(void* args){
    while(1){
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        

        char publish_msg[20];
        sprintf(publish_msg, "%d", random_number_generator(((publish_struct_t*)args)->base_value, ((publish_struct_t*)args)->deviation));
         
        pubmsg.payload = publish_msg;
        pubmsg.payloadlen = strlen(publish_msg);
        MQTTClient_publishMessage(atlasMQTTclient, ((publish_struct_t*)args)->feature, &pubmsg, &token);
        MQTTClient_waitForCompletion(atlasMQTTclient, token, TIMEOUT);
        char buff[256];
        sprintf(buff, "DP: Message %s with delivery token %d delivered.", publish_msg, token);
        ATLAS_LOGGER_DEBUG(buff);
        sleep(((publish_struct_t*)args)->publish_rate);
    }
    
    return NULL;
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
    deliveredtoken = dt;
}


int msgarrvd(void *context, char *topicName, int topicLen, 
	    MQTTClient_message *message){
            
    //printf("Message arrived\n");
    //printf("     topic: %s\n", topicName);
    //printf("   message: %s\n", message->payload);
   
    atlas_pkt_received(message->payloadlen);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

MQTTClient start_MQTTclient(char *topics){
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    char buff[256];

    if(MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) 
		!= MQTTCLIENT_SUCCESS){
        ATLAS_LOGGER_ERROR("Failed to create MQTTclient.");
    }
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        ATLAS_LOGGER_ERROR("Failed to connectMQTTclient");
    }
    
    char *p = strtok(topics, ",");
    
    while(p ){
        MQTTClient_subscribe(client, p, QOS);
        
        sprintf(buff, "Subscribing to topic %s for client %s using QOS %d\n", p, CLIENTID, QOS);
        ATLAS_LOGGER_DEBUG(buff);
        p = strtok(NULL, ",");
    }
       
    return client;
}

int verify_arguments(int argc, char** argv){
    if(!(argc < 5 || strcmp(argv[1], "--publish") || strcmp(argv[3], "--subscribe")))
        return 1; 
    return 0;
} 

static void traffic_generator(MQTTClient atlasMQTTclient, char* str){
    
    pthread_t pThread[10];
    publish_struct_t *publish_str; 
    int index = 0;
    
    char *p = strtok(str, ":");
    while(p){
        
        publish_str = (publish_struct_t*)malloc(sizeof(publish_struct_t));
        publish_str->feature = strdup (p);
        p = strtok(NULL, ":");
        publish_str->publish_rate = atoi (p);
        p = strtok(NULL, ":");
        publish_str->base_value = atoi (p);
        p = strtok(NULL, ":");
        publish_str->deviation = atoi (p);
        pthread_create(&pThread[index], NULL, publish, (void*)publish_str);
        
        p = strtok(NULL, ":");
        index++;
    }
}

static void print_usage()
{
    printf("Usage: ./data_plane --publish \"feature1:X1:Y1:Z1, feature2:X2:Y2:Z2\" --subscribe \"feature1, feature2\"\n");
}


int main(int argc, char *argv[])
{
    
        if(verify_arguments(argc, argv)){
            atlas_init( "username", "clientid", 10);
        
            /* start MQTT client */
            atlasMQTTclient = start_MQTTclient(argv[4]);
        
            traffic_generator(atlasMQTTclient, argv[2]);
        }  
        else{
            print_usage();
            return 0;
        } 

    while(1);
    
    return 0;
}
