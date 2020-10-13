#ifndef __ATLAS_COMMAND_EXECUTE_TYPE_H__
#define __ATLAS_COMMAND_EXECUTE_TYPE_H__

typedef enum _atlas_cmdExec_type
{
    /* Restart command: payload is empty*/
    ATLAS_CMD_DEVICE_RESTART = 0,

    /* Shutdown command: payload is empty*/
    ATLAS_CMD_DEVICE_SHUTDOWN,

    /* Unknown command: payload is empty*/
    ATLAS_CMD_DEVICE_UNKNOWN,
} atlas_cmdExec_type_t;

#endif /* __ATLAS_COMMAND_EXECUTE_TYPE_H__ */