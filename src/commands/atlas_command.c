#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include "atlas_command.h"

atlas_cmd_batch_t *
atlas_cmd_batch_new()
{
    atlas_cmd_batch_t *cmd_batch = (atlas_cmd_batch_t*) malloc(sizeof(atlas_cmd_batch_t));

    cmd_batch->length_ = 0;
    cmd_batch->buf_ = 0;

    return cmd_batch;
}

void
atlas_cmd_batch_free(atlas_cmd_batch_t *cmd_batch)
{
    if (!cmd_batch)
        return;

    if (cmd_batch->buf_)
        free(cmd_batch->buf_);

    free(cmd_batch);
}

void
atlas_cmd_batch_add(atlas_cmd_batch_t *cmd_batch, uint16_t type,
                    uint16_t length, const uint8_t* value)
{
    uint16_t new_len;
    uint8_t *p;
    
    if (!cmd_batch)
        return;

    /* Expand buffer */
    new_len = cmd_batch->length_ + 2 * sizeof(uint16_t) + length;
    cmd_batch->buf_ = (uint8_t *) realloc(cmd_batch->buf_, new_len);

    p = cmd_batch->buf_ + cmd_batch->length_;

    /* Add value */
    memcpy(p + 2 * sizeof(uint16_t), value, length);

    /* Add type */
    type = htons(type);
    memcpy(p, &type, sizeof(type));
    p += sizeof(type);
    /* Add length */
    length = htons(length);
    memcpy(p, &length, sizeof(length));

    cmd_batch->length_ = new_len;
}

void
atlas_cmd_batch_get_buf(atlas_cmd_batch_t *cmd_batch, uint8_t **buf, uint16_t *len)
{
    if (!cmd_batch)
        return;

    *buf = cmd_batch->buf_;
    *len = cmd_batch->length_;
}

