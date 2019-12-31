#ifndef __ATLAS_COAP_RESPONSE_H__
#define __ATLAS_COAP_RESPONSE_H__

typedef enum _atlas_coap_response
{
    ATLAS_COAP_RESP_OK = 200,
    ATLAS_COAP_RESP_NOT_FOUND = 404,
    ATLAS_COAP_RESP_TIMEOUT,
    ATLAS_COAP_RESP_RESET,

} atlas_coap_response_t;

#endif /* __ATLAS_COAP_RESPONSE_H__ */

