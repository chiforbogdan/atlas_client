#ifndef __ATLAS_STATUS_H__
#define __ATLAS_STATUS_H__

typedef enum _atlas_status {
    ATLAS_OK = 0,
    ATLAS_GENERAL_ERR,
    ATLAS_INVALID_HOSTNAME,
    ATLAS_INVALID_PORT,
    ATLAS_COAP_SRV_DTLS_NOT_SUPPORTED,
    ATLAS_COAP_INVALID_URI,
    ATLAS_INVALID_CALLBACK,

} atlas_status_t ;

#endif /* __ATLAS_STATUS_H__ */
