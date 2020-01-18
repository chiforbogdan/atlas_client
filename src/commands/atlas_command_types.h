#ifndef __ATLAS_COMMAND_TYPE_H__
#define __ATLAS_COMMAND_TYPE_H__

typedef enum _atlas_cmd_type
{
    /* Register command: payload is the client identity*/
    ATLAS_CMD_REGISTER = 0,

    /* Keepalive command: payload is a 2 byte token*/
    ATLAS_CMD_KEEPALIVE,

    /* Identity command: payload is the client identity */
    ATLAS_CMD_IDENTITY,

} atlas_cmd_type_t;

#endif /* __ATLAS_COMMAND_TYPE_H__ */
