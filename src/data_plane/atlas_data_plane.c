#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "atlas_client.h"
#include "MQTTClient.h"
#include "../logger/atlas_logger.h"

#define ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC "atlas/request/"
#define ATLAS_DATA_PLANE_CONSUME_TOPIC "atlas/"
#define ATLAS_DATA_PLANE_REQUEST_REPUTATION_MSG "REQUEST"
#define ATLAS_DATA_PLANE_BUFFER_LEN (256)
#define ATLAS_DATA_PLANE_REPUTATION_WINDOW_SEC (10)
#define ATLAS_DATA_PLANE_MESSAGE_MAX_LEN (20)

#define ATLAS_CLIENTID_MAX_LEN (64)

volatile MQTTClient_deliveryToken deliveredtoken;
volatile MQTTClient clientMQTT;
MQTTClient_message pubmsg = MQTTClient_message_initializer;
MQTTClient atlasMQTTclient;
int flag_reputation = 0;
static pthread_t wait_t;

char* clientid;
uint16_t qos, ppm, maxlen;
clock_t window_start;
char* feature = NULL;
uint16_t feature_value = 0;

char clientid_[ATLAS_CLIENTID_MAX_LEN];

typedef struct publish_struct {
    char* feature;
    int publish_rate;
    int base_value;
    int deviation;
} publish_struct_t;

static feedback_struct_t *feedback_entry = NULL;

static void feedback_struct_add(char* payload, uint16_t time_ms, char *feature);
static void* waiting_feature_values();

static int random_number_generator(int base_value, int deviation) 
{
    srand(time(0));
    int randInterval = (2 * deviation) + 1; //generate value only in the interval of -/+ deviation
    int lower = base_value - deviation; //limit the minimum value
    return ((random() % randInterval) + lower);
}

void*
publish(void* args)
{
    char* buff = NULL;
    char* publish_msg = NULL;
 
    while(1) {
        buff = malloc(ATLAS_DATA_PLANE_BUFFER_LEN + 1);
        snprintf(buff, ATLAS_DATA_PLANE_BUFFER_LEN, "atlas/%s", ((publish_struct_t*)args)->feature);
        
        publish_msg = malloc(ATLAS_DATA_PLANE_MESSAGE_MAX_LEN + 1);
        snprintf(publish_msg, ATLAS_DATA_PLANE_MESSAGE_MAX_LEN, "%d", random_number_generator(((publish_struct_t*)args)->base_value, ((publish_struct_t*)args)->deviation));

        pubmsg.qos = qos;
        pubmsg.retained = 0;
        pubmsg.payload = publish_msg;
        pubmsg.payloadlen = strlen(publish_msg);
        MQTTClient_publishMessage(atlasMQTTclient, buff, &pubmsg, NULL);
        
        free(buff);
        free(publish_msg);

        sleep(((publish_struct_t*)args)->publish_rate);
    }    
    return NULL;
}

static void
request_feature_values(const char* feature)
{     
    char *topic = NULL;
    MQTTClient_message rep_msg = MQTTClient_message_initializer;

    topic = malloc(strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC) + strlen(feature) + 1);
    strncpy(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC, strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC));
    strncat(topic, feature, strlen(feature));

    /* Send a request to topic "atlas/request/{feature}" in order to get the
    most recent {feature} sensor value from all devices from the network */
    rep_msg.qos = 2;
    rep_msg.retained = 0;
    rep_msg.payload = ATLAS_DATA_PLANE_REQUEST_REPUTATION_MSG;
    rep_msg.payloadlen = strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_MSG);
    MQTTClient_publishMessage(atlasMQTTclient, topic, &rep_msg, NULL); 

    free(topic);
 
    window_start = clock();

    pthread_create(&wait_t, NULL, &waiting_feature_values, NULL);
}

static void*
waiting_feature_values()
{
    printf("Reputation window start\n");
    /* Wait until the reputation window ends */
    sleep(ATLAS_DATA_PLANE_REPUTATION_WINDOW_SEC);
    printf("Reputation window end\n");
    
    init_feedback_command(feedback_entry);

    return NULL;
}

void 
publish_feature_value(char* topicName){
    char *buff = malloc(ATLAS_DATA_PLANE_BUFFER_LEN + 1);
    snprintf(buff, ATLAS_DATA_PLANE_BUFFER_LEN, "%s:%u", clientid, feature_value);
   
    pubmsg.qos = qos;
    pubmsg.retained = 0;
    pubmsg.payload = buff;
    pubmsg.payloadlen = strlen(buff);
    MQTTClient_publishMessage(atlasMQTTclient, topicName, &pubmsg, NULL);

    free(buff);
}

void
subscribe(MQTTClient client, const char* topic)
{
    MQTTClient_subscribe(client, topic, qos);    
    printf("Subscribing to topic %s for client %s.\n", topic, clientid);

    char *buff = malloc(ATLAS_DATA_PLANE_BUFFER_LEN + 1);
    snprintf(buff, ATLAS_DATA_PLANE_BUFFER_LEN, "Subscribing to topic %s for client %s using QOS %d\n", topic, clientid, qos);
    
    ATLAS_LOGGER_DEBUG(buff);
    
    free(buff);
}


void 
connlost(void *context, char *cause)
{
    char *buff = malloc(ATLAS_DATA_PLANE_BUFFER_LEN + 1);
    snprintf(buff, ATLAS_DATA_PLANE_BUFFER_LEN, "Connection lost, cause: %s.", cause);

    ATLAS_LOGGER_DEBUG(buff);

    free(buff);
}


void 
delivered(void *context, MQTTClient_deliveryToken dt)
{
    deliveredtoken = dt;
}


int 
msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    uint16_t response_time;
    char *payloadptr;
    char *reqTopic;
    char *featureTopic;
    char *p;

    /* Feed atlas client with ingress packet information */
    atlas_pkt_received(message->payloadlen);

    payloadptr = (char *) malloc(message->payloadlen + 1);
    memcpy(payloadptr, message->payload, message->payloadlen);
    payloadptr[message->payloadlen] = 0;

    /* If message is received on topic atlas/request/{feature}, then publish immediately a new value for {feature} */
    if (feature) {
        p = strstr(topicName, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
        if (p && p == topicName) {
            /* Skip to {feature} pointer */
            p += strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC); 
            if (p && !strcmp(p, feature)) {
                /* TODO publish value */
                printf("Publish value on request for feature %s\n", feature);
                return 1;
            }
        }
    }

    /* If message is received on topic atlas/{feature}, then consume the value */
    if(feature) {
        featureTopic = (char *) malloc(strlen(feature) + 7);
        strcpy(featureTopic, "atlas/");
        strcat(featureTopic, feature);
        if(strcmp(topicName, featureTopic) == 0) 
            feature_value = atoi ( message->payload);
    }

    if (strstr(topicName, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC)) {
        reqTopic = (char*) malloc (strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC) + strlen(feature) + 1);
        strcpy(reqTopic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
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

                window_start = clock() - window_start;
                response_time = (uint16_t) ((((double)window_start)/CLOCKS_PER_SEC)*1000);
                feedback_struct_add(payloadptr, response_time, feature);
            }
        }
    }
    
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

MQTTClient start_MQTTclient(char *topics, char* serverURI)
{
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    if(MQTTClient_create(&client, serverURI, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) 
        ATLAS_LOGGER_ERROR("Failed to create MQTTclient.");
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        ATLAS_LOGGER_ERROR("Failed to connectMQTTclient");
    }
    
    char *p = strtok(topics, ",");
    
    while(p) {
        /* Subscribe for topic in order to consume values */
        strcpy(topic, ATLAS_DATA_PLANE_CONSUME_TOPIC);
        strcat(topic, p);
        subscribe(client, topic);
        
        p = strtok(NULL, ",");
    }
       
    return client;
}

int parse_arguments(int argc, char** argv){
    if ((argc < 18) || (strcmp(argv[1], "--publish") != 0) 
                    || (strcmp(argv[3], "--subscribe") != 0) 
                    || (strcmp(argv[5], "--serverURI") != 0)
                    || (strcmp(argv[7], "--clientid") != 0)
                    || (strcmp(argv[9], "--qos") != 0)
                    || (strcmp(argv[11], "--ppm") != 0)
                    || (strcmp(argv[13], "--maxlen") != 0)
                    || (strcmp(argv[15], "--reputation") != 0))
        return 0; 

    clientid = strdup(argv[8]);
    qos = atoi(argv[10]);
    ppm = atoi(argv[12]);
    maxlen = atoi(argv[14]);
    feature = strdup(argv[16]);
    if (strcmp(feature, "0") != 0)
        flag_reputation = 1;

    return 1;
} 

static void traffic_generator(MQTTClient atlasMQTTclient, char* str){
    
    pthread_t pThread[10];
    publish_struct_t *publish_str; 
    int index = 0;
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];
   
    
    char *p = strtok(str, ":");
    while(p) {        
        publish_str = (publish_struct_t*)malloc(sizeof(publish_struct_t));
        publish_str->feature = strdup (p);
        p = strtok(NULL, ":");
        publish_str->publish_rate = atoi (p);
        p = strtok(NULL, ":");
        publish_str->base_value = atoi (p);
        p = strtok(NULL, ":");
        publish_str->deviation = atoi (p);
        pthread_create(&pThread[index], NULL, publish, (void*)publish_str);

        strcpy(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
        strcat(topic, publish_str->feature);
        printf("Subscribe to topic %s\n", topic);
        subscribe(atlasMQTTclient, topic);
 
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
    if(parse_arguments(argc, argv)) {                
        atlas_init( "username", clientid, qos, ppm, maxlen);
        /* start MQTT client */
        atlasMQTTclient = start_MQTTclient(argv[4], argv[6]);    
        traffic_generator(atlasMQTTclient, argv[2]);

        if (flag_reputation) {
            sleep(5);
            atlas_reputation_request(feature, clientid_, sizeof(clientid_));
            request_feature_values(feature);
        }
    } else {
        print_usage();
        return 0;
    } 

    while(1);
    
    return 0;
}
