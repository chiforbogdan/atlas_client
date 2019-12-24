#ifndef __ATLAS_SCHEDULER_H__
#define __ATLAS_SCHEDULER_H__

typedef void (*atlas_sched_cb_t)(int);

/**
* @brief Add scheduler entry
* @param[in] fd File descriptor
* @param cb Callback
* @return none
*/
void atlas_sched_add_entry(int fd, atlas_sched_cb_t cb);

/**
 * @brief Delete scheduler entry
 * @param[in] fd File descriptor
 * @return none
 */
void atlas_sched_del_entry(int fd);

/**
* @brief Run scheduler main loop
* @return none
*/
void atlas_sched_loop();

#endif /* __ATLAS_SCHEDULER_H__*/
