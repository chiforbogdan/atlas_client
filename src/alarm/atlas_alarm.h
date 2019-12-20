#ifndef __ATLAS_ALARM_H__
#define __ATLAS_ALARM_H__

#include <stdint.h>
#include "../scheduler/atlas_scheduler.h"

typedef int atlas_alarm_id_t;

typedef void (*atlas_alarm_cb_t)(atlas_alarm_id_t);

/**
* @brief Add alarm
* @param[in] ms Alarm expire time (ms)
* @param]in] alarm_cb Alarm callback
* @param[in] run_once Indicates if the alarm should run only once
* @return alarm id
*/
atlas_alarm_id_t atlas_alarm_set(uint16_t ms, atlas_alarm_cb_t alarm_cb, uint8_t run_once);

#endif /* __ATLAS_ALARM_H__ */
