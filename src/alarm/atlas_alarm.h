#ifndef __ATLAS_ALARM_H__
#define __ATLAS_ALARM_H__

#include <stdint.h>
#include "../scheduler/atlas_scheduler.h"

#define ATLAS_ALARM_RUN_ONCE           (1)
#define ATLAS_ALARM_RUN_MULTIPLE_TIMES (0)
#define ATLAS_ALARM_SEC_TO_MS(SEC)     ((SEC) * 1000)

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

/**
 * @brief Cancel alarm
 * @param[in] alarm_id Alarm id
 * @return none
 */
void atlas_alarm_cancel(atlas_alarm_id_t alarm_id);

#endif /* __ATLAS_ALARM_H__ */
