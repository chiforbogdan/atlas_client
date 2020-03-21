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
#define ATLAS_CLIENTID_MAX_LEN (64)
#define ATLAS_PUBLISH_FEATURES_MAX (32)
#define ATLAS_PACKET_LENGTH_DEFAULT "default"
#define ATLAS_REPUTATION_FEEDBACK_AVERAGE "average"
#define ATLAS_DATA_PLANE_FEEDBACK_MAX (256)
#define ATLAS_SEC_TO_MS(TIME) ((TIME) * 1000)

typedef struct publish_info
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
    /* Reputation lock */
    pthread_mutex_t lock;
} publish_info_t;

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
    /* Thread used to run the reputation query task */
    pthread_t thread;
    /* Reputation lock */
    pthread_mutex_t lock;
    /* Indicates if feedback window is active */
    int feedback_active; 
    /* Sensor value */
    float sensor_val[ATLAS_DATA_PLANE_FEEDBACK_MAX];
    /* Feedback score */
    atlas_feedback_t feedback[ATLAS_DATA_PLANE_FEEDBACK_MAX];
    /* Feedback score length */
    int feedback_length;
    /* Feedback window start timestamp */
    struct timespec window_start;
} reputation_t;

/* MQTT client */
static MQTTClient atlasMQTTclient;
/* Publish info */
static publish_info_t publish_str[ATLAS_PUBLISH_FEATURES_MAX];
/* Reputation info */
static reputation_t rep_info;
/* ATLAS Identity */
static char *identity;

static float
random_number_generator(int base_value, int deviation) 
{
    srand(time(0));
    int randInterval = (2 * deviation) + 1; //generate value only in the interval of -/+ deviation
    int lower = base_value - deviation; //limit the minimum value
    return (randInterval * ((float)random() / (float)RAND_MAX) + lower);
}

static void 
publish_feature_value(publish_info_t *pub_info)
{
    char topic[ATLAS_DATA_PLANE_BUFFER_LEN + 1];
    MQTTClient_message pub = MQTTClient_message_initializer;
    float value;
    int len;
    
    /* Create topic */
    sprintf(topic, "%s%s", ATLAS_DATA_PLANE_DATA_TOPIC, pub_info->feature);

    pthread_mutex_lock(&pub_info->lock);    
    
    /* If packet length is default, then deliver the simulated value, otherwise deliver the generate value  */
    if (!pub_info->packet_length) {
        value = random_number_generator(pub_info->base_value, pub_info->deviation);
        sprintf(pub_info->publish_msg, "%s:%f", identity, value);
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

    pthread_mutex_unlock(&pub_info->lock);    
    
    printf("TX: transmitted value %f for feature %s (topic %s)\n", value, pub_info->feature, topic);
}

static void*
publish_task(void* args)
{
    publish_info_t *pub_info = (publish_info_t*) args;
 
    while(1) {
        publish_feature_value(pub_info);
        sleep(pub_info->publish_rate);
    }

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

static uint16_t
get_sensor_feedback(double value)
{
    if (value < 0.2)
        return 98;
    else if (value < 0.6)
        return 95;
    else if (value < 0.8)
        return 90;
    else if (value < 1)
        return 85;
    else if (value < 1.3)
        return 80;
    else if (value < 1.5)
        return 50;
    else if (value < 2)
        return 35;
    else if (value < 2.5)
        return 20;
    else if (value < 3)
        return 10;

    return 0;
}

static long
diff_in_ms(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }

    return (diff.tv_sec * 1000.0 + diff.tv_nsec / 1000000.0);
}

static void
handle_data(const char *feature, const char *payload, size_t payload_len)
{
    char msg[ATLAS_DATA_PLANE_BUFFER_LEN] = { 0 };
    char clientid[ATLAS_DATA_PLANE_BUFFER_LEN] = { 0 };
    struct timespec ts;
    double value;
    double resp_time;
    int feedback_active;
    char *p;
    int i;

    if (!feature || !payload || !payload_len)
        return;

    memcpy(msg, payload, payload_len);
    
    p = strtok(msg, ":");
    /* Extract the client id */
    if (!p)
        return;
    strcpy(clientid, p);

    /* Extract the feature value */
    p = strtok(NULL, ":");
    if (!p)
        return;
    value = atof(p);

    printf("RX: received value %f for feature %s\n", value, feature);

    /* Verify if feedback window is active */
    pthread_mutex_lock(&rep_info.lock);
    feedback_active = rep_info.feedback_active;

    if (!feedback_active)
        goto EXIT;

    if (rep_info.feedback_length > ATLAS_DATA_PLANE_FEEDBACK_MAX)
        goto EXIT;

    /* If this identity delivered feedback in this window, then keep only the first entry */
    for (i = 0; i < rep_info.feedback_length; i++)
        if (!strcmp(clientid, rep_info.feedback[i].identity)) {
            printf("Discarding value for client with identity %s (device has already sent a sample)\n", clientid);
            goto EXIT;
        }

    /* Set sensor value */
    rep_info.sensor_val[rep_info.feedback_length] = value;

    /* Set feedback identity */
    strcpy(rep_info.feedback[rep_info.feedback_length].identity, clientid);

    /* Set feedback sensor type */
    rep_info.feedback[rep_info.feedback_length].sensor_type = ATLAS_SENSOR_TEMPERATURE; 

    /* If target value is set, then compute the sensor score according to the target value */
    if (rep_info.target_value != -1) {
        value -= rep_info.target_value;
        if (value < 0)
            value *= -1;
        rep_info.feedback[rep_info.feedback_length].sensor_score = get_sensor_feedback(value); 
    }

    /* Add response time feedback score */
    clock_gettime(CLOCK_REALTIME, &ts);
    resp_time = diff_in_ms(rep_info.window_start, ts);
    resp_time /= ATLAS_SEC_TO_MS(rep_info.window_size);
    rep_info.feedback[rep_info.feedback_length].time_score = (100 - resp_time * 100);

    rep_info.feedback_length++;

EXIT:
    pthread_mutex_unlock(&rep_info.lock);
}

static int 
msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    char *p;
    char *topic;

    if (!topicLen)
        topic = topicName;
    else {
        topic = (char *)malloc(topicLen + 1);
        memcpy(topic, topicName, topicLen);
        topic[topicLen] = 0;
    }

    /* Feed atlas client with ingress packet information */
    atlas_pkt_received(message->payloadlen);

    /* If message is received on topic atlas/request/{feature}, then publish immediately a new value for {feature} */
    p = strstr(topic, ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC);
    if (p && p == topic) {
        /* Skip to {feature} pointer */
        p += strlen(ATLAS_DATA_PLANE_REQUEST_REPUTATION_TOPIC); 
        handle_publish_req(p);
        goto EXIT;
    }

    /* If message is received on topic atlas/data/{feature}, then consume the value */
    p = strstr(topic, ATLAS_DATA_PLANE_DATA_TOPIC);
    if(p && p == topic) {
        /* Skip to {feature} pointer */
        p += strlen(ATLAS_DATA_PLANE_DATA_TOPIC);
        handle_data(p, message->payload, message->payloadlen);
    }
    
EXIT:
    if (topicLen)
        free(topic);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    
    return 1;
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
        /* Extract feature */
        strcpy(publish_str[index].feature, p);
        p = strtok(NULL, ":");
        /* Extract publish rate */
        publish_str[index].publish_rate = atoi(p);
        p = strtok(NULL, ":");
        /* Extract simulated base value */
        publish_str[index].base_value = atoi(p);
        p = strtok(NULL, ":");
        /* Extract simulated deviation value */
        publish_str[index].deviation = atoi(p);
        p = strtok(NULL, ":");
        /* Extract publish QoS value */
        publish_str[index].qos = atoi(p);
        p = strtok(NULL, ":");
        /* Extract simulated packet length */
        if (!strcmp(p, ATLAS_PACKET_LENGTH_DEFAULT)) {
            publish_str[index].packet_length = 0;
	    publish_str[index].publish_msg = (char *) malloc(ATLAS_DATA_PLANE_BUFFER_LEN + 1);
	} else {
            publish_str[index].packet_length = atoi(p);
	    publish_str[index].publish_msg = (char *) malloc(publish_str[index].packet_length);
	}

        /* Start a new thread for each published feature */	
	pthread_mutex_init(&publish_str[index].lock, NULL);
        pthread_create(&publish_str[index].thread, NULL, publish_task, (void*) (publish_str + index));

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

static void*
reputation_query_task(void *arg)
{
    int i;
    double avg;
    double val;
    char *identity;

    while(1) {
        identity = NULL;
        if (atlas_reputation_request(ATLAS_SENSOR_TEMPERATURE, &identity) != ATLAS_OK)
            printf("Error in getting an identity with best reputation\n");

        if (identity)
            printf("Identity with the best reputation is %s\n", identity);

        request_reputation_val(rep_info.feature);

        /* Start feedback window */
        pthread_mutex_lock(&rep_info.lock);
        rep_info.feedback_active = 1;
        pthread_mutex_unlock(&rep_info.lock);
        
        printf("Start reputation feedback window\n");
        rep_info.feedback_length = 0;
        clock_gettime(CLOCK_REALTIME, &rep_info.window_start);
 
        /* Sleep for the window size time */
        sleep(rep_info.window_size);
        
        /* End feedback window */
        pthread_mutex_lock(&rep_info.lock);
        rep_info.feedback_active = 0;
        pthread_mutex_unlock(&rep_info.lock);

        printf("Stop reputation feedback window\n");
        
        /* If there is no target value, compute the average sensor feedback */
        if (rep_info.target_value == -1 && rep_info.feedback_length) {
            avg = 0;
            for (i = 0; i < rep_info.feedback_length; i++)
                avg += rep_info.sensor_val[i];
            
            avg /= rep_info.feedback_length;

            for (i = 0; i < rep_info.feedback_length; i++) {
                val = rep_info.sensor_val[i] -  avg;
                if (val < 0)
                    val *= -1;
                rep_info.feedback[i].sensor_score = get_sensor_feedback(val);
            }
        }

        for (i = 0; i < rep_info.feedback_length; i++)
            printf("EVENT: Send feedback for identity %s :sensor score %d response time score %d\n",
                   rep_info.feedback[i].identity, rep_info.feedback[i].sensor_score, rep_info.feedback[i].time_score);

        if (rep_info.feedback_length &&
            atlas_reputation_feedback(rep_info.feedback, rep_info.feedback_length) != ATLAS_OK)
            printf("Error in sending reputation feedback!\n");
        
        /* Consume data from the most trusted device */
        if (identity) {
            for (i = 0; i < rep_info.feedback_length; i++)
                if (!strcmp(identity, rep_info.feedback[i].identity)) {
                    printf("EVENT: Consume data from device with identity %s: sensor value %f\n", identity, rep_info.sensor_val[i]);
                    break;
                }
        }

        rep_info.feedback_length = 0;
        
        free(identity);

        /* Sleep for the remaining of the reputation query rate */
        sleep(rep_info.query_rate - rep_info.window_size);
    }

    return NULL;
}

static void
set_reputation(char *reputation_arg)
{
    char *p;

    memset(&rep_info, 0, sizeof(rep_info));

    p = strtok(reputation_arg, ":");
    if(p) {
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

        /* Start reputation query thread */
        pthread_mutex_init(&rep_info.lock, NULL);
        pthread_create(&rep_info.thread, NULL, reputation_query_task, NULL);
    }
}

static void
print_usage()
{
    printf("Usage: \n");
    printf(" --publish \"<sensor feature>:<publish rate in seconds>:<target value>:<deviation>:<qos>:<default | forced packet length>\"\n");
    printf(" --subscribe \"<sensor feature1>:<sensor feature2>\"\n");
    printf(" --hostname protocol://host:port\n");
    printf(" --qos-firewall <qos>\n");
    printf(" --ppm-firewall <ppm>\n");
    printf(" --maxlen-firewall <maxlen>\n");
    printf(" --reputation \"<subscribed sensor feature>:<query rate in seconds>:<window size in seconds>:<average | target value>\"\n");
}

static void
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

    if (!publish_arg || !subscribe_arg || !hostname_arg) {
        print_usage();
	exit(1);
    }

    if (qos == -1 || ppm == -1 || maxlen == -1) {
        print_usage();
	exit(1);
    }

    /* Set firewall rule and init connection with atlas client */
    if (atlas_init(qos, ppm, maxlen, &identity) != ATLAS_OK) {
        printf("Error in initializing connection with ATLAS client\n");
        exit(1);
    }
    
    /* start MQTT client */
    atlasMQTTclient = start_MQTTclient(identity, hostname_arg);    
    /* Setup traffic generator */
    traffic_generator(identity, publish_arg);
    /* Subscribe to consume topics */
    subscribe_topics(identity, subscribe_arg, qos);
    /* Set reputation */
    if (reputation_arg)
        set_reputation(reputation_arg);
} 

int main(int argc, char *argv[])
{ 
    parse_arguments(argc, argv);

    while(getchar() != 'q');
 
    return 0;
}
