#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "../scheduler/atlas_scheduler.h"
#include "../logger/atlas_logger.h"
#include "../commands/atlas_command.h"
#include "../commands/atlas_command_types.h"
#include "received_commands.h"


char *socket_path = "\0hidden";
struct sockaddr_un addr;
char buf[100];
char *buffer;
int fd,cl,rc;

static void
callback(int fd)
{ 
	printf("sunt in callback\n");
	atlas_status_t status = ATLAS_OK;
	
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      perror("accept error");
    }
	
    while ( (rc=read(cl,buf,sizeof(buf))) > 0) {
		printf("read %u bytes: %s\n", rc, buf);
      
		atlas_cmd_batch_t *cmd_batch;
		const atlas_cmd_t *cmd;
		uint8_t push_found = 0;
		
		cmd_batch = atlas_cmd_batch_new();
		status = atlas_cmd_batch_set_raw(cmd_batch, (uint8_t*)buf, rc);
		
		if (status != ATLAS_OK) {
			ATLAS_LOGGER_ERROR("Corrupted command from data plane");
			printf("Corrupted command from data plane\n");
			status = ATLAS_CORRUPTED_COMMAND;
		}
		
		cmd = atlas_cmd_batch_get(cmd_batch, NULL);
		while (cmd) {
			if (cmd->type == ATLAS_CMD_DATA_PLANE_USERNAME ) {
				printf("Am primit USERNAME\n");
			}
			else 
				if (cmd->type == ATLAS_CMD_DATA_PLANE_CLIENTID ) {
					printf("Am primit CLIENTID\n");
				}
				else
					if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_PER_MINUTE ) {
						printf("Am primit POLICY PACKETS_PER_MINUTE\n");
					}
					else
					if (cmd->type == ATLAS_CMD_DATA_PLANE_POLICY_PACKETS_AVG ) {
						printf("Am primit POLICY PACKETS_AVG\n");
					}
					else
					if (cmd->type == ATLAS_CMD_DATA_PLANE_PACKETS_PER_MINUTE ) {
						printf("Am primit PACKETS_PER_MINUTE\n");
					}
					else
					if (cmd->type == ATLAS_CMD_DATA_PLANE_PACKETS_AVG ) {
						printf("Am primit PACKETS_AVG\n");
					}

			cmd = atlas_cmd_batch_get(cmd_batch, cmd);
		}
		atlas_cmd_batch_free(cmd_batch);
    }
    if (rc == -1) {
      perror("read");
      
    }
    else if (rc == 0) {
      printf("EOF\n");
      
    }
    return status;
} 

static void 
bind_socket(){
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}
	
	int rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	while (rc == -1){
		sleep(1);
		perror("bind error");
		rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	}
}

static void 
listen_socket(){
	int rc = listen(fd, 5);
	while(rc == -1)
	{
		perror("listen error");
		rc = listen(fd, 5);
	}
}

int 
atlas_receive_commands_start()
{
	
   memset(&addr, 0, sizeof(addr));
   addr.sun_family = AF_UNIX;
   if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
	} 
	else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
		unlink(socket_path);
	}
  
  bind_socket();
 
  listen_socket();

  atlas_sched_add_entry(fd, callback);
  
 
  return fd;
}


