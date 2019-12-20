#include "atlas_logger.h"
#ifdef __linux__
#include<syslog.h>
#endif /* __linux__ */

#define ATLAS_CLIENT "ATLAS_CLIENT"

#ifdef __linux__
int
convert_to_syslog_severity(atlas_log_severity_t severity)
{
    switch(severity) {
        case ATLAS_LOG_NOTICE:
            return LOG_NOTICE;

        case ATLAS_LOG_INFO:
            return LOG_INFO;

        case ATLAS_LOG_DEBUG:
            return LOG_DEBUG;

        default:
            return LOG_ALERT;
    }

    return LOG_ALERT;
}
#endif /* __linux__ */

void
atlas_log_init()
{
#ifdef __linux__
    openlog (ATLAS_CLIENT, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
#endif /* __linux__ */
}

void
atlas_log(atlas_log_severity_t severity, const char *msg)
{
    int logger_severity;

    if (!msg)
        return;

#ifdef __linux__
    logger_severity = convert_to_syslog_severity(severity);
    syslog(logger_severity, "%s", msg);
#endif /* __linux__ */
}

void
atlas_log_close()
{
#ifdef __linux__
    closelog();
#endif /* __linux__ */
}

