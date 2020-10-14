#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "atlas_command_execute_utils.h"
#include "atlas_command_execute_engine.h"
#include "atlas_command_execute_types.h"
#include "../commands/atlas_command.h"
#include "../logger/atlas_logger.h"
#include "../alarm/atlas_alarm.h"

#define ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS (3000)

static atlas_cmd_exec_t *rcvdCmd = NULL;

/* Flag for monitoring in progress commands. */
/* TO DO: update if implementing a layered structure for execution commands. */
static bool cmdInProgress = false;


static void
command_execution_shutdown_alarm_callback()
{
    ATLAS_LOGGER_INFO("Shutdown command execution alarm callback");
    atlas_command_execute_shutdown();
}

static void
command_execution_restart_alarm_callback()
{
    ATLAS_LOGGER_INFO("Restart command execution alarm callback");
    atlas_command_execute_restart();
}

static void
command_execution_unknown_alarm_callback()
{
    ATLAS_LOGGER_INFO("Unknown command execution alarm callback");
    cmdInProgress = false;
    atlas_command_execute_unknown(rcvdCmd);
}

atlas_status_t 
atlas_alert_command_execution_parse(const uint8_t *buf, uint16_t buf_len)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    atlas_status_t status = ATLAS_OK;

    if (cmdInProgress) {
        ATLAS_LOGGER_DEBUG("Command execution alert end-point deferred a command");
        status = ATLAS_DEFERRED_COMMAND_EXECUTION;
        return status;
    }

    if (!buf || !buf_len)
        return ATLAS_INVALID_INPUT;

    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Corrupted command execution alert payload");
        status = ATLAS_CORRUPTED_COMMAND_EXECUTION_PAYLOAD;
	goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        if (cmd->type == ATLAS_CMD_DEVICE_RESTART) {            
            if (atlas_alarm_set(ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS, command_execution_restart_alarm_callback, 
                                ATLAS_ALARM_RUN_ONCE) < 0)
                ATLAS_LOGGER_ERROR("Error in scheduling a restart command execution alarm!");
            cmdInProgress = true;
	    break;
        } else if (cmd->type == ATLAS_CMD_DEVICE_SHUTDOWN) {            
            if (atlas_alarm_set(ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS, command_execution_shutdown_alarm_callback, 
                                ATLAS_ALARM_RUN_ONCE) < 0)
                ATLAS_LOGGER_ERROR("Error in scheduling a shutdown command execution alarm!");
            cmdInProgress = true;
	    break;
        } else if (cmd->type == ATLAS_CMD_DEVICE_UNKNOWN && cmd->length >= 0) {
            if (!rcvdCmd) 
                rcvdCmd = (atlas_cmd_exec_t*) malloc(sizeof(atlas_cmd_exec_t));
            rcvdCmd->type = cmd->type;
            rcvdCmd->value = (char*) malloc((cmd->length * sizeof(char)) + 1);
            memcpy(rcvdCmd->value, cmd->value, cmd->length);
            memset(rcvdCmd->value+cmd->length, 0, 1);
            if (atlas_alarm_set(ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS, command_execution_unknown_alarm_callback, 
                                ATLAS_ALARM_RUN_ONCE) < 0)
                ATLAS_LOGGER_ERROR("Error in scheduling an unknown command execution alarm!");
            cmdInProgress = true;
	    break;
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }   

EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}