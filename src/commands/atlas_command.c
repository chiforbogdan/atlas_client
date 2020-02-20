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

atlas_status_t
atlas_cmd_batch_set_raw(atlas_cmd_batch_t *cmd_batch, const uint8_t *buf, uint16_t len)
{
    const uint8_t *p = buf;
    uint16_t tmp_len = len;
    uint16_t cmd_len;

    if (!cmd_batch || !buf || !len)
        return ATLAS_INVALID_INPUT;

    /* Validate buffer */
    while(tmp_len > 0) {
        /* Validate type */
        if (tmp_len < sizeof(uint16_t)){
            printf("eroare1 \n");

            return ATLAS_CORRUPTED_COMMAND;
        }
        
        p += sizeof(uint16_t);
        tmp_len -= sizeof(uint16_t);
    
        /* Validate length */
        if (tmp_len < sizeof(uint16_t)){
                        printf("eroare2 \n");

            return ATLAS_CORRUPTED_COMMAND;
            
        }

        memcpy(&cmd_len, p, sizeof(uint16_t));
        cmd_len = ntohs(cmd_len);
        p += sizeof(uint16_t);
        tmp_len -= sizeof(uint16_t);
    
        /* Validate value */
        if (tmp_len < cmd_len){
            printf("eroare3 \n");
            return ATLAS_CORRUPTED_COMMAND;
        }

        p += cmd_len;
        tmp_len -= cmd_len; 
    }

    cmd_batch->buf_ = malloc(sizeof(uint8_t) * len);
    memcpy(cmd_batch->buf_, buf, len);
    cmd_batch->length_ = len;

    return ATLAS_OK;
}

const atlas_cmd_t*
atlas_cmd_batch_get(atlas_cmd_batch_t *cmd_batch, const atlas_cmd_t *current_cmd)
{
    atlas_cmd_t *cmd;
    uint8_t *p = (uint8_t *) current_cmd;

    if (!cmd_batch)
        return NULL;

    if (!p)
        cmd = (atlas_cmd_t *) cmd_batch->buf_;
    else {
        /* Jump to next command */
        p += 2 * sizeof(uint16_t) + current_cmd->length;
        /* If end of buffer is reached */
        if ((p - cmd_batch->buf_) >= cmd_batch->length_)
            return NULL;

        cmd = (atlas_cmd_t *) p;
    }

    cmd->type = ntohs(cmd->type);
    cmd->length = ntohs(cmd->length);

    return cmd;
}
