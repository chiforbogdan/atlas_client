#ifndef __ATLAS_COMMAND_EXECUTE_ENGINE_H__
#define __ATLAS_COMMAND_EXECUTE_ENGINE_H__

#include "atlas_command_execute.h"

/**
 * @brief Execute a shutdown command
 * @param[in] cmd Command received from gateway
 * @return none
 */ 
void atlas_command_execute_shutdown();

/**
 * @brief Execute a restart command
 * @param[in] cmd Command received from gateway
 * @return none
 */
void atlas_command_execute_restart();

/**
 * @brief Execute payload from an unknown command type
 * @param[in] cmd Command received from gateway
 * @return none
 */
void atlas_command_execute_unknown(const atlas_cmd_exec_t *cmd);

#endif /* __ATLAS_COMMAND_EXECUTE_ENGINE_H__ */