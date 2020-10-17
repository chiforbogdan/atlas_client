#include <stdlib.h>
#include "atlas_command_execute_engine.h"
#include "../logger/atlas_logger.h"

#define ATLAS_EXECUTE_UNKNOWN_COMMAND "ATLAS_COMMAND_EXECUTE: Executing command: "

void atlas_command_execute_shutdown() {
    ATLAS_LOGGER_INFO("ATLAS_COMMAND_EXECUTE: Executing shutdown command...");
    system("systemctl poweroff");
}

void atlas_command_execute_restart() {
    ATLAS_LOGGER_INFO("ATLAS_COMMAND_EXECUTE: Executing restart command...");
    system("systemctl reboot");
}