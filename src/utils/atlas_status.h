#ifndef __ATLAS_STATUS_H__
#define __ATLAS_STATUS_H__

typedef enum _atlas_status {
    ATLAS_OK = 0,
    ATLAS_GENERAL_ERR,
    ATLAS_INVALID_HOSTNAME,
    ATLAS_INVALID_PORT,
    ATLAS_COAP_SRV_DTLS_NOT_SUPPORTED,
} atlas_status_t ;

#endif /* __ATLAS_STATUS_H__ */
