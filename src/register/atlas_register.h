#ifndef __ATLAS_REGISTER_H__
#define __ATLAS_REGISTER_H__

#include <stdint.h>

/**
 * @brief Start registration and keep-alive process
 * @return none
 */
void atlas_register_start();

/**
* @brief Indicates if the client is registered to gateway
* @return non-zero if client is registered to gateway, 0 otherwise
*/
uint8_t atlas_is_registered();

#endif /* __ATLAS_REGISTER_H__ */
