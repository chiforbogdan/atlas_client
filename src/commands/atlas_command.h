#ifndef __ATLAS_COMMAND_H__
#define __ATLAS_COMMAND_H__

#include <stdint.h>
#include "../utils/atlas_status.h"

typedef struct _atlas_cmd
{
    /* Command type */
    uint16_t type;

    /* Command length */
    uint16_t length;

    /* Command value */
    uint8_t value[0];
} atlas_cmd_t;

typedef struct _atlas_cmd_batch
{
    /* Internal buffer length */
    uint16_t length_;

    /* Internal buffer */
    uint8_t *buf_;
} atlas_cmd_batch_t;

/**
* @brief Create new command batch
* @return new command batch
*/
atlas_cmd_batch_t *atlas_cmd_batch_new();

/**
* @brief Free a command batch
* @param[in] cmd_batch Command batch to be deleted
* @return none
*/
void atlas_cmd_batch_free(atlas_cmd_batch_t *cmd_batch);

/**
* @brief Add command to command batch
* @param[in] cmd_batch Command batch
* @param[in] type Command type
* @param[in] length Command length
* @param[in] value Command value
* @return none
*/
void atlas_cmd_batch_add(atlas_cmd_batch_t *cmd_batch, uint16_t type, uint16_t length, const uint8_t *value);

/**
* @brief Get serialized command batch
* @param[in] cmd_batch Command batch
* @param[out] buf Serialized buffer
* @param[out] len Serialized buffer length
* @return none
*/
void atlas_cmd_batch_get_buf(atlas_cmd_batch_t *cmd_batch, uint8_t **buf, uint16_t *len);

/**
* @brief Set raw command buffer in command batch (parse commands)
* @param[in] cmd_batch Command batch
* @param[in] buf Raw command buffer
* @param[in] len Raw command buffer length
* @return status
*/
atlas_status_t atlas_cmd_batch_set_raw(atlas_cmd_batch_t *cmd_batch, const uint8_t *buf, uint16_t len);

/**
* @brief Get command from command batch
* @param[in] cmd_batch Command batch
* @param[in] current_cmd Current command
* @return New command from command batch
*/
const atlas_cmd_t* atlas_cmd_batch_get(atlas_cmd_batch_t *cmd_batch, const atlas_cmd_t *current_cmd);

#endif /* __ATLAS_COMMAND_H__ */
