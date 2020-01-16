#include <stdlib.h>
#include "atlas_command.h"

atlas_cmd_batch_t *
atlas_cmd_batch_new()
{
    atlas_cmd_batch_t *cmd_batch = (atlas_cmd_batch_t*) malloc(sizeof(atlas_cmd_batch_t));

    cmd_batch->length = 0;
    cmd_batch->buf = 0;

    return cmd_batch;
}

void
atlas_cmd_batch_free(atlas_cmd_batch_t *cmd_batch)
{
    if (!cmd_batch)
        return;

    if (cmd_batch->buf)
        free(cmd_batch->buf);

    free(cmd_batch);
}

void
atlas_cmd_batch_add(atlas_cmd_batch_t *cmd_batch, uint16_t type,
                    uint16_t length, const uint8_t* value)
{
    if (!cmd_batch)
        return;

    /* Expand buffer */
    cmd_batch->length += 2 * sizeof(uint16_t) + length;
    cmd_batch->buf = (uint8_t *) realloc(cmd_batch->buf, cmd_batch->length);
}
