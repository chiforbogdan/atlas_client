#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "client.h"
#include "MQTTClient.h"

int main(int argc, char *argv[])
{
    pthread_t init_t;
    atlas_init(init_t, "username", "clientID", "policy");
    
    //CLIENT MQTT
    MQTTClient atlasMQTTClient;
    atlasMQTTClient = start_MQTTclient();
    
    while(1){
        
    }
    pthread_join(init_t, NULL);
    return 0;
}
