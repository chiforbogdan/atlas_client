#ifndef __ATLAS_COMMAND_EXECUTE_H__
#define __ATLAS_COMMAND_EXECUTE_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct _atlas_cmd_exec
{
    /* Command type */
    uint16_t type;

    /* Command payload */
    char *value;
} atlas_cmd_exec_t;

/**
* @brief Init command execution engine
* @return True if a new CoAP resource is successfully added to CoAP server, False otherwise
*/
bool atlas_command_execute_init();


#endif /* __ATLAS_COMMAND_EXECUTE_H__ */ 
