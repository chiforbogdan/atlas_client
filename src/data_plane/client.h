#ifndef CLIENT_H
#define CLIENT_H


#include "MQTTClient.h"
#include <stdint.h>

void atlas_init(char* user, char* client_id, uint16_t pol);
	    
MQTTClient start_MQTTclient();


#endif

