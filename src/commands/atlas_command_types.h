#ifndef __ATLAS_COMMAND_TYPE_H__
#define __ATLAS_COMMAND_TYPE_H__

typedef enum _atlas_cmd_type
{
    /* Register command: payload is the client identity, response is empty */
    ATLAS_CMD_REGISTER,

    /* Keepalive command: payload is a 2 byte token, response is empty */
    ATLAS_CMD_KEEPALIVE,

} atlas_cmd_type_t;

#endif /* __ATLAS_COMMAND_TYPE_H__ */
