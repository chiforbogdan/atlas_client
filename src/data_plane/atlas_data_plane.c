#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atlas_client.h"
#include "MQTTClient.h"
#include "../logger/atlas_logger.h"

#define ADDRESS "tcp://127.0.0.1:1883"
#define CLIENTID "client"
#define TOPIC "TOPIC"
#define PAYLOAD "PAYLOAD"
#define QOS 1
#define TIMEOUT 1000L

volatile MQTTClient_deliveryToken deliveredtoken;
volatile MQTTClient clientMQTT;

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
   
    atlas_pkt_received(message->payloadlen);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

MQTTClient start_MQTTclient(){
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

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
      
    MQTTClient_subscribe(client, TOPIC, QOS);
    printf("Subscribing to topic %s for client %s using QOS %d\n", TOPIC, CLIENTID, QOS);
       
    return client;
}

int main(int argc, char *argv[])
{
    
    atlas_init( "username", "clientid", 10);
    
    //CLIENT MQTT
    MQTTClient atlasMQTTClient;
    atlasMQTTClient = start_MQTTclient();
    
    while(1){
        
    }
    
    return 0;
}
