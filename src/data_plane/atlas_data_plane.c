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
#define ATLAS_DATA_PLANE_DATA_TOPIC "atlas/data/"
#define ATLAS_DATA_PLANE_REQUEST_REPUTATION_MSG "REQUEST"
#define ATLAS_DATA_PLANE_FEATURE_MAX_LEN (32)
#define ATLAS_DATA_PLANE_BUFFER_LEN (256)
#define ATLAS_DATA_PLANE_REPUTATION_WINDOW_SEC (10)
#define ATLAS_DATA_PLANE_MESSAGE_MAX_LEN (20)
#define ATLAS_CLIENTID_MAX_LEN (64)
#define ATLAS_PUBLISH_FEATURES_MAX (32)

typedef struct publish_struct {
    char feature[ATLAS_DATA_PLANE_FEATURE_MAX_LEN];
    int publish_rate;
    int base_value;
    int deviation;
} publish_struct_t;

int flag_reputation = 0;
static pthread_t wait_t;

char* clientid;
uint16_t qos, ppm, maxlen;
clock_t window_start;
char* feature = NULL;
uint16_t feature_value = 0;

char clientid_[ATLAS_CLIENTID_MAX_LEN];

static MQTTClient atlasMQTTclient;
static pthread_t pThread[ATLAS_PUBLISH_FEATURES_MAX];
static publish_struct_t publish_str[ATLAS_PUBLISH_FEATURES_MAX];

static feedback_struct_t *feedback_entry = NULL;

static void feedback_struct_add(char* payload, uint16_t time_ms, char *feature);
static void* waiting_feature_values();

static float
random_number_generator(int base_value, int deviation) 
{
    srand(time(0));
    int randInterval = (2 * deviation) + 1; //generate value only in the interval of -/+ deviation
    int lower = base_value - deviation; //limit the minimum value
    return (randInterval * ((float)random() / (float)RAND_MAX) + lower);
}

static void 
publish_feature_value(publish_struct_t *pub_info)
{
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN + 1];
    char publish_msg[ATLAS_DATA_PLANE_MESSAGE_MAX_LEN + 1];
    MQTTClient_message pub = MQTTClient_message_initializer;
    float value;

    sprintf(topic, "%s%s", ATLAS_DATA_PLANE_DATA_TOPIC, pub_info->feature);
    
    value = random_number_generator(pub_info->base_value, pub_info->deviation);
    sprintf(publish_msg, "%f", value);
        
    pub.qos = qos; 
    pub.retained = 0;
    pub.payload = publish_msg;
    pub.payloadlen = strlen(publish_msg);
    MQTTClient_publishMessage(atlasMQTTclient, topic, &pub, NULL);

    printf("TX: transmitted value %f for feature %s\n", value, pub_info->feature);
}

static void*
publish(void* args)
{
    publish_struct_t *pub_info = (publish_struct_t*) args;
 
    while(1) {
        publish_feature_value(pub_info);

        sleep(pub_info->publish_rate);
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

static void
subscribe(MQTTClient client, const char* topic)
{
    MQTTClient_subscribe(client, topic, qos);  
    printf("Subscribing to topic %s for client %s.\n", topic, clientid);
}

static void
handle_publish_req(const char *feature)
{
    int i;

    if (!feature)
        return;

    for (i = 0; i < sizeof(publish_str) / sizeof(publish_str[0]); i++)
        if (!strcmp(feature, publish_str[i].feature)) {
            publish_feature_value(&publish_str[i]);
            break;
        }
}

static void
handle_data(const char *feature, const char *payload, size_t payload_len)
{
    char msg[ATLAS_DATA_PLANE_BUFFER_LEN] = { 0 };
    double value;

    if (!feature || !payload || !payload_len)
        return;

    memcpy(msg, payload, payload_len);
    value = atof(msg);

    printf("RX: received value %f for feature %s\n", value, feature);
}

static int 
msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    uint16_t response_time;
    char *reqTopic;
    char *featureTopic;
    char *p;

    /* Feed atlas client with ingress packet information */
    atlas_pkt_received(message->payloadlen);

    /* If message is received on topic atlas/request/{feature}, then publish immediately a new value for {feature} */
    p = strstr(topicName, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
    if (p && p == topicName) {
        /* Skip to {feature} pointer */
        p += strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC); 
        handle_publish_req(p);
        goto EXIT;
    }

    /* If message is received on topic atlas/{feature}, then consume the value */
    p = strstr(topicName, ATLAS_DATA_PLANE_DATA_TOPIC);
    if(p && p == topicName) {
        /* Skip to {feature} pointer */
        p += strlen(ATLAS_DATA_PLANE_DATA_TOPIC);
        handle_data(p, message->payload, message->payloadlen);
    }
    
EXIT:
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    
    return 1;
}

static void
feedback_struct_add(char* payload, uint16_t time_ms, char *feature) {
    
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

    if(MQTTClient_create(&client, serverURI, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) { 
        printf("Failed to create MQTTclient\n");
        return client;
    }
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, NULL, msgarrvd, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connectMQTTclient\n");
        return client;
    }
    
    char *p = strtok(topics, ",");
    while(p) {
        /* Subscribe for topic in order to consume values */
        strcpy(topic, ATLAS_DATA_PLANE_DATA_TOPIC);
        strcat(topic, p);
        subscribe(client, topic);
        
        p = strtok(NULL, ",");
    }
       
    return client;
}

static void
traffic_generator(MQTTClient atlasMQTTclient, char* str)
{
    int index = 0;
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];
    
    char *p = strtok(str, ":");
    while(p) {        
        strcpy(publish_str[index].feature, p);
        p = strtok(NULL, ":");
        publish_str[index].publish_rate = atoi(p);
        p = strtok(NULL, ":");
        publish_str[index].base_value = atoi(p);
        p = strtok(NULL, ":");
        publish_str[index].deviation = atoi(p);
        pthread_create(&pThread[index], NULL, publish, (void*) (publish_str + index));

        /* Subscribe to atlas/request/{feature} in order to server values for {feature} on demand */
        strcpy(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
        strcat(topic, publish_str[index].feature);
        subscribe(atlasMQTTclient, topic);
 
        p = strtok(NULL, ":");
        index++;
    }
}

static int
parse_arguments(int argc, char** argv)
{
    if ((argc < 17) || (strcmp(argv[1], "--publish") != 0) 
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

static void
print_usage()
{
    printf("Usage: ./data_plane");
    printf(" --publish \"<sensor feature>:<publish rate in seconds>:<target value>:<deviation>\"");
    printf(" --subscribe \"<sensor feature1>, <sensor feature2>\"");
    printf(" --serverURI protocol://host:port");
    printf(" --clientid <clientid>");
    printf(" --qos <qos>");
    printf(" --ppm <ppm>");
    printf(" --maxlen <maxlen>");
    printf(" --reputation \"<sensor feature>:<query rate in seconds>:<target value>:<window size in seconds>\"");
    printf("\n");
}

int main(int argc, char *argv[])
{ 
    if(!parse_arguments(argc, argv)) {
        print_usage();
        return 0;
    }

    atlas_init(clientid, qos, ppm, maxlen);

    /* start MQTT client */
    atlasMQTTclient = start_MQTTclient(argv[4], argv[6]);    

    /* Setup traffic generator */
    traffic_generator(atlasMQTTclient, argv[2]);

    if (flag_reputation) {
        sleep(5);
        atlas_reputation_request(feature, clientid_, sizeof(clientid_));
        request_feature_values(feature);
    }

    while(1);
    
    return 0;
}
