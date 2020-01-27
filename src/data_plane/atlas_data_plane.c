#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "client.h"

int main(int argc, char *argv[])
{
    
    atlas_init("username", 1, "policy");

    pthread_t monitor_packets_t, send_alarm_t;

    pthread_create(&monitor_packets_t, NULL, &packets, NULL);
    pthread_create(&send_alarm_t, NULL, &send_values_to_atlas_client, NULL);
    pthread_join(monitor_packets_t, NULL);
    pthread_join(send_alarm_t, NULL);

    
    return 0;
}
