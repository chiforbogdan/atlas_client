#ifndef __ATLAS_DATA_PLANE_H__
#define __ATLAS_DATA_PLANE_H__

#include <stdint.h>

typedef struct feedback_struct {
    char* clientID;
    char* feature;
    uint16_t feature_value;
    uint16_t reponse_time;
    struct feedback_struct *next;
} feedback_struct_t;

#endif /* __ATLAS_DATA_PLANE_H__ */
