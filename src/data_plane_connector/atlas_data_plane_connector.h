#ifndef __ATLAS_DATA_PLANE_CONNECTOR_H__
#define __ATLAS_DATA_PLANE_CONNECTOR_H__

#include <stdint.h>
#include "../scheduler/atlas_scheduler.h"


atlas_status_t atlas_data_plane_connector_start();
uint16_t get_packets_per_min();
uint16_t get_packets_avg();



#endif /* __ATLAS_DATA_PLANE_CONNECTOR_H__ */
