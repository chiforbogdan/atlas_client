#include <sys/timerfd.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "atlas_alarm.h"
#include "../logger/atlas_logger.h"

#define ATLAS_1MS_TO_NS (1000000)
#define ATLAS_1SEC_TO_MS (1000)

typedef struct _atlas_alarm_entry
{
    /* Alarm id */
    atlas_alarm_id_t alarm_id;

    /* Alarm callback*/
    atlas_alarm_cb_t callback;

    /* Next alarm */
    struct _atlas_alarm_entry *next;

    /* Indicates if the alarm should be triggered only once */
    uint8_t run_once;
} atlas_alarm_entry_t;

static atlas_alarm_entry_t *alarm_entry;

static void
atlas_alarm_trigger(int fd)
{
    atlas_alarm_entry_t *p;
    uint64_t res;

    for(p = alarm_entry; p; p = p->next)
        if (p->alarm_id == fd) {
            read(fd, &res, sizeof(res));
            /* Run alarm callback */
            p->callback(p->alarm_id);

            /* Cancel alarm if it should run only once */
	    if (p->run_once) {
                ATLAS_LOGGER_DEBUG("Alarm which runs only once will be canceled");
                atlas_alarm_cancel(p->alarm_id);
	    }

            break;
        }
}

atlas_alarm_id_t
atlas_alarm_set(uint16_t ms, atlas_alarm_cb_t alarm_cb, uint8_t run_once)
{
    struct itimerspec time_spec;
    atlas_alarm_entry_t *alarm, *p;
    int fd;
    int res;

    memset(&time_spec, 0, sizeof(time_spec));
    
    fd = timerfd_create(CLOCK_REALTIME, 0);
    if (fd == -1)
        return -1;

    if (!run_once) {
        time_spec.it_interval.tv_sec = ms / ATLAS_1SEC_TO_MS;
        time_spec.it_interval.tv_nsec = (ms % ATLAS_1SEC_TO_MS) * ATLAS_1MS_TO_NS;
    }

    time_spec.it_value.tv_sec = ms / ATLAS_1SEC_TO_MS;
    time_spec.it_value.tv_nsec = (ms % ATLAS_1SEC_TO_MS) * ATLAS_1MS_TO_NS;

    res = timerfd_settime(fd, 0, &time_spec, NULL);
    if (res < 0) {
        close(fd);
        return -1;
    }

    /* Schedule alarm */
    atlas_sched_add_entry(fd, atlas_alarm_trigger);

    alarm = (atlas_alarm_entry_t *) malloc(sizeof(atlas_alarm_entry_t));
    alarm->alarm_id = fd;
    alarm->callback = alarm_cb;
    alarm->run_once = run_once;
    alarm->next = NULL;

    if (!alarm_entry)
        alarm_entry = alarm;
    else {
        p = alarm_entry;
        while(p->next) p = p->next;

        p->next = alarm;
    }
    
    return fd;
}

void
atlas_alarm_cancel(atlas_alarm_id_t alarm_id)
{
    atlas_alarm_entry_t *p, *pp;

    for(p = alarm_entry; p; p = p->next) {
        if (p->alarm_id == alarm_id) {
	    if (p == alarm_entry)
                alarm_entry = p->next;
	    else
               pp->next = p->next;

            ATLAS_LOGGER_DEBUG("Alarm is canceled");

            atlas_sched_del_entry(p->alarm_id);

            free(p);

	    break;
	}
        pp = p;
    }
}
