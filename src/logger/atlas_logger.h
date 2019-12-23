#ifndef __ATLAS_LOGGER_H__
#define __ATLAS_LOGGER_H__

typedef enum _atlas_log_severity
{
    ATLAS_LOG_NOTICE = 0,
    ATLAS_LOG_INFO,
    ATLAS_LOG_DEBUG,
    ATLAS_LOG_ERROR,

} atlas_log_severity_t;

void atlas_log_init();

void atlas_log(atlas_log_severity_t severity, const char *msg);

void atlas_log_close();

#define ATLAS_LOGGER_DEBUG(MSG) atlas_log(ATLAS_LOG_DEBUG, (MSG))
#define ATLAS_LOGGER_INFO(MSG) atlas_log(ATLAS_LOG_INFO, (MSG))
#define ATLAS_LOGGER_ERROR(MSG) atlas_log(ATLAS_LOG_ERROR, (MSG))

#endif /* __ATLAS_LOGGER_H__ */
