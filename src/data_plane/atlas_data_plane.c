#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "atlas_client.h"
#include "MQTTClient.h"
#include "../logger/atlas_logger.h"
#include "atlas_data_plane.h"

#define ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC "atlas/request"
#define ATLAS_DATA_PLANE_BUFFER_LEN (256)
#define ATLAS_DATA_PLANE_MQTT_TIMOUT_MS (5000)

volatile MQTTClient_deliveryToken deliveredtoken;
volatile MQTTClient clientMQTT;
MQTTClient_deliveryToken token = 0;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient atlasMQTTclient;
int flag_reputation = 0;
static pthread_t wait_t;

char* clientid;
uint16_t qos, ppm, maxlen;
clock_t t, t1;
char* feature = NULL;
uint16_t feature_value = 0;


typedef struct publish_struct {
    char* feature;
    int publish_rate;
    int base_value;
    int deviation;
} publish_struct_t;

static feedback_struct_t *feedback_entry = NULL;

static void feedback_struct_add(char* payload, uint16_t time_ms, char *feature);
static void* waiting_feature_values();
    
static int random_number_generator(int base_value, int deviation){
    int lower, upper;
    lower = base_value - deviation;
    upper = base_value + deviation;
    
    return random() % (upper - lower +deviation) + lower;
}


void* publish(void* args){
    while(1){

        char buff[ATLAS_DATA_PLANE_BUFFER_LEN];
        char publish_msg[20];
        sprintf(publish_msg, "%d", random_number_generator(((publish_struct_t*)args)->base_value, ((publish_struct_t*)args)->deviation));

	    sprintf(buff, "atlas/%s", ((publish_struct_t*)args)->feature);

        pubmsg.qos = qos;
        pubmsg.retained = 0;
        pubmsg.payload = publish_msg;
        pubmsg.payloadlen = strlen(publish_msg);
        MQTTClient_publishMessage(atlasMQTTclient, buff, &pubmsg, &token);
        
        sprintf(buff, "DP: Message %s with delivery token %d delivered.", publish_msg, token);
        //ATLAS_LOGGER_DEBUG(buff);
        sleep(((publish_struct_t*)args)->publish_rate);
    }
    
    return NULL;
}

void request_feature_values(const char* feature){
    
    const char* publish_msg = "value";
    
    char *topic = malloc (strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC) + strlen(feature) + 2);
    strcpy(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
    strcat(topic, "/");
    strcat(topic, feature);

    pubmsg.qos = qos;
    pubmsg.retained = 0;
    pubmsg.payload = publish_msg;
    pubmsg.payloadlen = strlen(publish_msg);
    MQTTClient_publishMessage(atlasMQTTclient, topic, &pubmsg, &token); 
    t = clock();
    t1 = clock();

    pthread_create(&wait_t, NULL, &waiting_feature_values, NULL);
    
}

static void* waiting_feature_values(){
    uint16_t time;
    clock_t t2;
    while (1){
        t2 = clock() - t1;
        time = (uint16_t) ((((double)t2)/CLOCKS_PER_SEC)*1000);
        if( time > ATLAS_DATA_PLANE_MQTT_TIMOUT_MS ){
            init_feedback_command(feedback_entry);
            break;
        }        
    }
    return NULL;
}

void publish_feature_value(char* topicName){
    char buff[ATLAS_DATA_PLANE_BUFFER_LEN];

    sprintf(buff, "%s:%u", clientid, feature_value);
   
    pubmsg.qos = qos;
    pubmsg.retained = 0;
    pubmsg.payload = buff;
    pubmsg.payloadlen = strlen(buff);
    MQTTClient_publishMessage(atlasMQTTclient, topicName, &pubmsg, &token);
}

void subscribe(MQTTClient client, char* topic){

	char buff[ATLAS_DATA_PLANE_BUFFER_LEN];
	sprintf(buff, "atlas/%s", topic);
	MQTTClient_subscribe(client, buff, qos);
    printf("Subscribing to topic atlas/%s for client %s.\n", topic, clientid);
	sprintf(buff, "Subscribing to topic atlas/%s for client %s using QOS %d\n", topic, clientid, qos);
	ATLAS_LOGGER_DEBUG(buff);
 	
}

void connlost(void *context, char *cause){
    char buff[ATLAS_DATA_PLANE_BUFFER_LEN];
    sprintf(buff, "Connection lost, cause: %s.", cause);
    ATLAS_LOGGER_DEBUG(buff);
}

void delivered(void *context, MQTTClient_deliveryToken dt){
    deliveredtoken = dt;
}


int msgarrvd(void *context, char *topicName, int topicLen, 
	    MQTTClient_message *message){

    uint16_t response_time;
    char *payloadptr;
    char *reqTopic;
    char *featureTopic;

    payloadptr = (char *) malloc(message->payloadlen + 1);
    memcpy(payloadptr, message->payload, message->payloadlen);
    payloadptr[message->payloadlen] = 0;

    if(feature != NULL){
        featureTopic = (char *) malloc(strlen(feature) + 7);
        strcpy(featureTopic, "atlas/");
        strcat(featureTopic, feature);
        if(strcmp(topicName, featureTopic) == 0) 
            feature_value = atoi ( message->payload);
    }

    if (strstr(topicName, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC) != NULL) {
        reqTopic = (char*) malloc (strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC) + strlen(feature) + 2);
        strcpy(reqTopic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
        strcat(reqTopic, "/");
        strcat(reqTopic, feature);
    }

    if( strcmp(topicName, reqTopic) == 0 ) {  
        if( strcmp(payloadptr, "value") == 0 ) {
            publish_feature_value(topicName);
        }
        else {
            if(flag_reputation == 1){
                printf("Message arrived\n");
                printf("     topic: %s\n", topicName);
                printf("   message: %s\n", payloadptr);

                t = clock() - t;
                response_time = (uint16_t) ((((double)t)/CLOCKS_PER_SEC)*1000);
                feedback_struct_add(payloadptr, response_time, feature);
            }
        }
    }

    atlas_pkt_received(message->payloadlen);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static void feedback_struct_add(char* payload, uint16_t time_ms, char *feature) {
    
    feedback_struct_t *ent, *p;
    char *tmp;

    ent = (feedback_struct_t*) malloc(sizeof(feedback_struct_t));
    
    tmp = strtok(payload, ":");
    ent->clientID = (char*) malloc (strlen (tmp) + 1);
    strcpy(ent->clientID, tmp);
    tmp = strtok(NULL, ":");
    ent->feature_value = atoi (tmp);
    ent->feature = (char*) malloc (strlen (feature) + 1);
    strcpy(ent->feature, feature);
    ent->reponse_time = time_ms;
    ent->next = NULL;

    if ( feedback_entry == NULL )
        feedback_entry = ent;
    else{
        p = feedback_entry;
        while(p->next != NULL ) 
            p = p->next;
        p->next = ent;
    }
}

MQTTClient start_MQTTclient(char *topics, char* serverURI) {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];

    if(MQTTClient_create(&client, serverURI, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL) 
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
        subscribe(client, p);

        //Subscribe at request/feature topic in order to send/receive values to/from other clients for reputation feedback
        strcpy(topic, "request/");
        strcat(topic, p);        
        subscribe(client, topic);

        p = strtok(NULL, ",");
    }
       
    return client;
}

int verify_arguments(int argc, char** argv){
    if(!(argc < 15 || strcmp(argv[1], "--publish") 
                    || strcmp(argv[3], "--subscribe") 
                    || strcmp(argv[5], "--serverURI")
                    || strcmp(argv[7], "--clientid")
                    || strcmp(argv[9], "--qos")
                    || strcmp(argv[11], "--ppm")
                    || strcmp(argv[13], "--maxlen")))
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
    printf("Usage: ./data_plane --publish \"feature1:X1:Y1:Z1, feature2:X2:Y2:Z2\" --subscribe \"feature1, feature2\" --serverURI protocol://host:port --clientid <clientid> --qos <qos> --ppm <ppm> --maxlen <maxlen> --reputation <feature>\n");

}


int main(int argc, char *argv[])
{
    
        if(verify_arguments(argc, argv)){
            
            clientid = strdup(argv[8]);
            qos = atoi(argv[10]);
            ppm = atoi(argv[12]);
            maxlen = atoi(argv[14]);
            atlas_init( "username", clientid, qos, ppm, maxlen);
            /* start MQTT client */
            atlasMQTTclient = start_MQTTclient(argv[4], argv[6]);
        
            traffic_generator(atlasMQTTclient, argv[2]);

             if (argc == 17){
                feature = strdup(argv[16]);
                sleep(5);
                atlas_reputation_request(feature);
                flag_reputation = 1;
            }
        }  
        else{
            print_usage();
            return 0;
        } 

    while(1);
    
    return 0;
}
