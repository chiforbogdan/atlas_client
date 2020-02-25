#include "../logger/atlas_logger.h"
#include "atlas_telemetry_hostname.h"
#include "atlas_telemetry_kern_info.h"
#include "atlas_telemetry_sysinfo.h"
#include "atlas_telemetry_packets_info.h"


void
atlas_telemetry_features_init()
{
    ATLAS_LOGGER_DEBUG("Init telemetry features...");

    /* Add hostname feature */
    atlas_telemetry_add_hostname();

    /* Add kernel info feature */
    atlas_telemetry_add_kern_info();

    /* Add sysinfo feature */
    atlas_telemetry_add_sysinfo();
    
    /*Add packets_info feature */
    atlas_telemetry_add_packets_info();
}
