#include <stdlib.h>
#include <string.h>
#include "atlas_command_execute_utils.h"
#include "atlas_command_execute_engine.h"
#include "atlas_command_execute_types.h"
#include "../commands/atlas_command.h"
#include "../logger/atlas_logger.h"
#include "../alarm/atlas_alarm.h"

#define ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS (0)     //timeout given in ms


/* Flag for monitoring in progress commands. */
/* TO DO: update if implementing a layered structure for execution commands. */
static bool cmdInProgress = false;


static void
command_execution_shutdown_alarm_callback()
{
    ATLAS_LOGGER_INFO("ATLAS_COMMAND_EXECUTE: Shutdown command execution alarm callback");
    atlas_command_execute_shutdown();
}

static void
command_execution_restart_alarm_callback()
{
    ATLAS_LOGGER_INFO("ATLAS_COMMAND_EXECUTE: Restart command execution alarm callback");
    atlas_command_execute_restart();
}


atlas_status_t 
atlas_alert_command_execution_parse(const uint8_t *buf, uint16_t buf_len)
{
    atlas_cmd_batch_t *cmd_batch;
    const atlas_cmd_t *cmd;
    atlas_status_t status = ATLAS_OK;

    if (cmdInProgress) {
        ATLAS_LOGGER_DEBUG("ATLAS_COMMAND_EXECUTE: Command execution alert end-point deferred a command");
        return ATLAS_DEFERRED_COMMAND_EXECUTION;
    }

    if (!buf || !buf_len)
        return ATLAS_INVALID_INPUT;

    cmd_batch = atlas_cmd_batch_new();

    status = atlas_cmd_batch_set_raw(cmd_batch, buf, buf_len);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("ATLAS_COMMAND_EXECUTE: Corrupted command execution alert payload");
        status = ATLAS_CORRUPTED_COMMAND_EXECUTION_PAYLOAD;
	goto EXIT;
    }

    cmd = atlas_cmd_batch_get(cmd_batch, NULL);
    while (cmd) {
        switch (cmd->type)
        {
        case ATLAS_CMD_DEVICE_RESTART:
            if (atlas_alarm_set(ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS, command_execution_restart_alarm_callback, 
                                ATLAS_ALARM_RUN_ONCE) < 0)
                ATLAS_LOGGER_ERROR("ATLAS_COMMAND_EXECUTE: Error in scheduling a restart command execution alarm!");
            else
                cmdInProgress = true;
            break;
        case ATLAS_CMD_DEVICE_SHUTDOWN:
            if (atlas_alarm_set(ATLAS_CLIENT_COMMAND_EXECUTE_TIMEOUT_MS, command_execution_shutdown_alarm_callback, 
                                ATLAS_ALARM_RUN_ONCE) < 0)
                ATLAS_LOGGER_ERROR("ATLAS_COMMAND_EXECUTE: Error in scheduling a shutdown command execution alarm!");
            else
                cmdInProgress = true;
        break;
        case ATLAS_CMD_DEVICE_UNKNOWN:
            ATLAS_LOGGER_INFO("ATLAS_COMMAND_EXECUTE: Unknown command execution received. Command execution alarm not scheduled!");
        break;
        }

        cmd = atlas_cmd_batch_get(cmd_batch, cmd);
    }   

EXIT:
    atlas_cmd_batch_free(cmd_batch);

    return status;
}