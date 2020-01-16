#ifndef __ATLAS_COMMAND_H__
#define __ATLAS_COMMAND_H__

#include <stdint.h>

typedef struct _atlas_cmd
{
    uint16_t type;

    uint16_t length;

    uint8_t value[0];
} atlas_cmd_t;

typedef struct _atlas_cmd_batch
{
    uint16_t length;

    uint8_t *buf;
} atlas_cmd_batch_t;

atlas_cmd_batch_t *atlas_cmd_batch_new();
void atlas_cmd_batch_free(atlas_cmd_batch_t *cmd_batch);

void atlas_cmd_batch_add(uint16_t type, uint16_t length, const uint8_t* value);

#endif /* __ATLAS_COMMAND_H__ */
