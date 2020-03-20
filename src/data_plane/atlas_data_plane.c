#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
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
#define ATLAS_PACKET_LENGTH_DEFAULT "default"
#define ATLAS_REPUTATION_FEEDBACK_AVERAGE "average"

typedef struct publish_struct
{
    /* Feature name */
    char feature[ATLAS_DATA_PLANE_FEATURE_MAX_LEN];
    /* Publish rate in seconds */
    int publish_rate;
    /* Base simulated feature value */
    int base_value;
    /* Deviation from base value */
    int deviation;
    /* QoS used when publishing the data */
    int qos;
    /* Packet length. If this value is non-zero a packet
     * with the indicate length is used, otherwise the packet will
     * contain the simulated value (used to simulate big packet length attacks) */
    int packet_length;
    /* Buffer used to hold the published messages */
    char *publish_msg;
    /* Thread used to run the publisher task */
    pthread_t thread;
} publish_struct_t;

typedef struct reputation
{
    /* Feature name */
    char feature[ATLAS_DATA_PLANE_FEATURE_MAX_LEN];
    /* Query rate in seconds */
    int query_rate;
    /* Target value use to evaluate the received value */
    int target_value;
    /* Window size in seconds. Used to measure the response time */
    int window_size;
} reputation_t;

/* MQTT client */
static MQTTClient atlasMQTTclient;
/* Publish info */
static publish_struct_t publish_str[ATLAS_PUBLISH_FEATURES_MAX];
/* Reputation info */
reputation_t rep_info;

static feedback_struct_t *feedback_entry = NULL;

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
    MQTTClient_message pub = MQTTClient_message_initializer;
    float value;
    int len;
    
    /* Create topic */
    sprintf(topic, "%s%s", ATLAS_DATA_PLANE_DATA_TOPIC, pub_info->feature);
    
    /* If packet length is default, then deliver the simulated value, otherwise deliver the generate value  */
    if (!pub_info->packet_length) {
        value = random_number_generator(pub_info->base_value, pub_info->deviation);
        sprintf(pub_info->publish_msg, "%f", value);
        len = strlen(pub_info->publish_msg);
    } else {
	memset(pub_info->publish_msg, '0', pub_info->packet_length);
	len = pub_info->packet_length;
    }
    
    pub.qos = pub_info->qos;
    pub.retained = 0;
    pub.payload = pub_info->publish_msg;
    pub.payloadlen = len;
    MQTTClient_publishMessage(atlasMQTTclient, topic, &pub, NULL);

    printf("TX: transmitted value %f for feature %s (topic %s)\n", value, pub_info->feature, topic);
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
request_reputation_val(const char* feature)
{     
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];
    MQTTClient_message rep_msg = MQTTClient_message_initializer;

    /* Set reputation request topic */
    strcpy(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
    strcat(topic, feature);

    /* Send a request to topic "atlas/request/{feature}" in order to get the
    most recent {feature} sensor value from all devices from the network */
    rep_msg.qos = 2;
    rep_msg.retained = 0;
    rep_msg.payload = ATLAS_DATA_PLANE_REQUEST_REPUTATION_MSG;
    rep_msg.payloadlen = strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_MSG);
    MQTTClient_publishMessage(atlasMQTTclient, topic, &rep_msg, NULL); 
}

static void
subscribe(const char* clientid, const char* topic, int qos)
{
    MQTTClient_subscribe(atlasMQTTclient, topic, qos);  
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

static MQTTClient
start_MQTTclient(const char *clientid, const char* hostname)
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    if(MQTTClient_create(&client, hostname, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) { 
        printf("Failed to create MQTTclient\n");
        exit(1);
    }
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, NULL, msgarrvd, NULL);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connectMQTTclient\n");
        exit(1);
    }
    
    return client;
}

static void
traffic_generator(char* clientid, char* publish_arg)
{
    int index = 0;
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];
    
    char *p = strtok(publish_arg, ":");
    while(p) {        
        strcpy(publish_str[index].feature, p);
        p = strtok(NULL, ":");
        publish_str[index].publish_rate = atoi(p);
        p = strtok(NULL, ":");
        publish_str[index].base_value = atoi(p);
        p = strtok(NULL, ":");
        publish_str[index].deviation = atoi(p);
        p = strtok(NULL, ":");
        publish_str[index].qos = atoi(p);
        p = strtok(NULL, ":");
        if (!strcmp(p, ATLAS_PACKET_LENGTH_DEFAULT)) {
            publish_str[index].packet_length = 0;
	    publish_str[index].publish_msg = (char *) malloc(ATLAS_DATA_PLANE_MESSAGE_MAX_LEN + 1);
	} else {
            publish_str[index].packet_length = atoi(p);
	    publish_str[index].publish_msg = (char *) malloc(publish_str[index].packet_length);
	}

        /* Start a new thread for each published feature */	
	pthread_create(&publish_str[index].thread, NULL, publish, (void*) (publish_str + index));

        /* Subscribe to atlas/request/{feature} in order to serve values for {feature} on demand */
        strcpy(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
        strcat(topic, publish_str[index].feature);
        subscribe(clientid, topic, 2);
 
        p = strtok(NULL, ":");
        index++;
    }
}

static void
subscribe_topics(char *clientid, char *consume_topics, int qos)
{
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN];
    char *p;
    
    p = strtok(consume_topics, ":");
    while(p) {
        /* Subscribe for topic in order to consume values */
        strcpy(topic, ATLAS_DATA_PLANE_DATA_TOPIC);
        strcat(topic, p);
        subscribe(clientid, topic, qos);
        
        p = strtok(NULL, ",");
    }
}

static void
set_reputation(char *reputation_arg)
{
    char *p;

    memset(&rep_info, 0, sizeof(rep_info));

    p = strtok(reputation_arg, ":");
    while(p) {
        /* Extract reputation feature */
        strcpy(rep_info.feature, p);
        /* Extract query rate */
        p = strtok(NULL, ":");
        rep_info.query_rate = atoi(p);
        /* Extract window size */
        p = strtok(NULL, ":");
        rep_info.window_size = atoi(p);
	if (rep_info.window_size >= rep_info.query_rate) {
            printf("Window size cannot exceed query rate\n");
            exit(1);
	}
        /* Extract target value */
        p = strtok(NULL, ":");
        if (!strcmp(p, ATLAS_REPUTATION_FEEDBACK_AVERAGE))
            rep_info.target_value = -1;
	else
            rep_info.target_value = atoi(p);
 
        p = strtok(NULL, ",");
    }
}

static void
print_usage()
{
    printf("Usage: \n");
    printf(" --publish \"<sensor feature>:<publish rate in seconds>:<target value>:<deviation>:<qos>:<default | forced packet length>\"\n");
    printf(" --subscribe \"<sensor feature1>:<sensor feature2>\"\n");
    printf(" --hostname protocol://host:port\n");
    printf(" --qos <qos>\n");
    printf(" --ppm <ppm>\n");
    printf(" --maxlen <maxlen>\n");
    printf(" --reputation \"<subscribed sensor feature>:<query rate in seconds>:<window size in seconds>:<average | target value>\"\n");
}

static int
parse_arguments(int argc, char** argv)
{
    int c;
    char *publish_arg = NULL;
    char *subscribe_arg = NULL;
    char *hostname_arg = NULL;
    char *reputation_arg = NULL;
    int qos = -1;
    int ppm = -1;
    int maxlen = -1;
    struct option longopts[] = {
        { "publish", required_argument, 0, 'p'},
        { "subscribe", required_argument, 0, 's'},
        { "hostname", required_argument, 0, 'h'},
        { "qos-firewall", required_argument, 0, 'q'},
        { "ppm-firewall", required_argument, 0, 'P'},
        { "maxlen-firewall", required_argument, 0, 'm'},
        { "reputation", required_argument, 0, 'r'},
	{ 0, 0, 0, 0 }
    };

    while ((c = getopt_long(argc, argv, "p:s:h:q:P:m:r:", longopts, NULL)) != -1) {
        switch (c) {
            case 'p':
                publish_arg = optarg;
                break;
	    case 's':
                subscribe_arg = optarg;
                break;
            case 'h':
                hostname_arg = optarg;
                break;
            case 'q':
                qos = atoi(optarg);
                break;
            case 'P':
                ppm = atoi(optarg);
                break;
            case 'm':
                maxlen = atoi(optarg);
                break;
            case 'r':
                reputation_arg = optarg;
                break;
	    default:
                print_usage();
		exit(1);
	}
    }

    if (!publish_arg || !subscribe_arg || !hostname_arg || !reputation_arg) {
        print_usage();
	exit(1);
    }

    if (qos == -1 || ppm == -1 || maxlen == -1) {
        print_usage();
	exit(1);
    }

    /* Set firewall rule and init connection with atlas client TODO get clientid from atlas */
    atlas_init("test", qos, ppm, maxlen);

    /* start MQTT client */
    atlasMQTTclient = start_MQTTclient("test", hostname_arg);    

    /* Setup traffic generator */
    traffic_generator("test", publish_arg);

    /* Subscribe to consume topics */
    subscribe_topics("test", subscribe_arg, qos);

    /* Set reputation */
    set_reputation(reputation_arg);

    return 1;
} 

int main(int argc, char *argv[])
{ 
    char clientid[100];    
    parse_arguments(argc, argv);
   
    atlas_reputation_request("ddd", clientid, sizeof(clientid));
    request_reputation_val("dddd");

    while(1);
    
    return 0;
}
