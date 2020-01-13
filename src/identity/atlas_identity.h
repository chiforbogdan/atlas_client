#ifndef __ATLAS_IDENTITY_H__
#define __ATLAS_IDENTITY_H__

#include "../utils/atlas_status.h"

/**
* @brief Init identity module
* @return status
*/
atlas_status_t atlas_identity_init();

/**
* @brief Get identity
* @return identity
*/
const char *atlas_identity_get();

/**
* @brief Get Pre-shared key
* @return pre-shared key
*/ 
const char *atlas_psk_get();

#endif /* __ATLAS_IDENTITY_H__ */
