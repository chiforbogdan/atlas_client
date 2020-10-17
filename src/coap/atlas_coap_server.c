#include <stdio.h>
#include <coap2/coap.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "atlas_coap_server.h"
#include "../logger/atlas_logger.h"
#include "../scheduler/atlas_scheduler.h"

#define ATLAS_COAP_DEFAULT_STRING "ATLAS client CoAP server"

#define ATLAS_MAX_PSK_KEY_BYTES (64)

#define ATLAS_COAP_SERVER_IS_UDP(MODE) ((MODE) & ATLAS_COAP_SERVER_MODE_UDP)
#define ATLAS_COAP_SERVER_IS_DTLS(MODE) ((MODE) & ATLAS_COAP_SERVER_MODE_DTLS_PSK)

typedef struct _atlas_coap_server_listener
{
    /* CoAP resource */
    coap_resource_t *resource;

    /* High layer application callback */
    atlas_coap_server_cb_t callback;
    
    /* CoAP URI path */
    char *uri_path;

    /* Next pointer in list */
    struct _atlas_coap_server_listener *next;
} atlas_coap_server_listener_t;

/* Pre-shared key */
static uint8_t key[ATLAS_MAX_PSK_KEY_BYTES + 1];

/* CoAP server context */
static coap_context_t *ctx;
static int fd;

static atlas_coap_server_listener_t *server_listeners[ATLAS_COAP_METHOD_MAX];

static int
set_dtls_psk(coap_context_t *ctx, const char *psk_val)
{
    coap_dtls_spsk_t psk;

    if (!coap_dtls_is_supported()) {
        ATLAS_LOGGER_ERROR("CoAP DTLS is not supported");
        return ATLAS_COAP_SRV_DTLS_NOT_SUPPORTED;
    }

    memset(&psk, 0, sizeof(psk));
    psk.version = COAP_DTLS_SPSK_SETUP_VERSION;

    strncpy((char *) key, psk_val, ATLAS_MAX_PSK_KEY_BYTES);
    psk.psk_info.key.s = key;
    psk.psk_info.key.length = strlen((char *) key);

    /* Set PSK */
    coap_context_set_psk2(ctx, &psk);

    return 0;
}

static coap_context_t*
get_context(uint16_t port, atlas_coap_server_mode_t server_mode, const char *psk)
{
    coap_context_t *ctx = NULL;
    coap_address_t listen_addr, listen_addrs;
    uint16_t tmp;
    coap_endpoint_t *ep = NULL;
    coap_endpoint_t *eps = NULL;

    ctx = coap_new_context(NULL);
    if (!ctx)
        return NULL;

    /* Set DTLS PSK */
    if (ATLAS_COAP_SERVER_IS_DTLS(server_mode))
        set_dtls_psk(ctx, psk);
        
    coap_address_init(&listen_addr);
    listen_addr.addr.sa.sa_family = AF_INET;
    listen_addr.addr.sin.sin_port = htons (port);
    listen_addrs = listen_addr;

    if (ATLAS_COAP_SERVER_IS_UDP(server_mode) && ATLAS_COAP_SERVER_IS_DTLS(server_mode)) {
        if (listen_addr.addr.sa.sa_family == AF_INET) {
            tmp = ntohs(listen_addr.addr.sin.sin_port) + 1;
            listen_addrs.addr.sin.sin_port = htons(tmp);
        } else if (listen_addr.addr.sa.sa_family == AF_INET6) {
            uint16_t temp = ntohs(listen_addr.addr.sin6.sin6_port) + 1;
            listen_addrs.addr.sin6.sin6_port = htons(temp);
        }
    }

    if (ATLAS_COAP_SERVER_IS_UDP(server_mode)) {
        ep = coap_new_endpoint(ctx, &listen_addr, COAP_PROTO_UDP);
        if (!ep) {
            ATLAS_LOGGER_ERROR("Cannot open COAP UDP");
            goto ERROR;
        }
    }

    if (ATLAS_COAP_SERVER_IS_DTLS(server_mode)) {
        eps = coap_new_endpoint(ctx, &listen_addrs, COAP_PROTO_DTLS);
        if (!eps) {
            ATLAS_LOGGER_ERROR("Cannot open COAP DTLS");
            goto ERROR;
        }
    }

    return ctx;

ERROR:
    coap_free_endpoint(ep);
    coap_free_endpoint(eps);
    coap_free_context(ctx);

    return NULL;
}

static void
get_default_index_handler(coap_context_t *ctx,
                  struct coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request,
                  coap_binary_t *token,
                  coap_string_t *query,
                  coap_pdu_t *response) {
 
    const char *INDEX = ATLAS_COAP_DEFAULT_STRING;

    coap_add_data_blocked_response(resource, session, request, response, token,
                                   COAP_MEDIATYPE_TEXT_PLAIN, 0x2ffff,
                                   strlen(INDEX),
                                   (const uint8_t *)INDEX);
}

static void
init_default_resources(coap_context_t *ctx)
{
    coap_resource_t *resource;

    resource = coap_resource_init(NULL, 0);
    coap_register_handler(resource, COAP_REQUEST_GET, get_default_index_handler);
    coap_add_resource(ctx, resource);
}

static void
atlas_coap_server_sched_callback(int fd)
{ 
    ATLAS_LOGGER_DEBUG("Serving CoAP request...");
    
    coap_io_process(ctx, COAP_IO_NO_WAIT);
} 

static int
atlas_coap_get_method(atlas_coap_method_t method)
{
    switch(method) {
        case ATLAS_COAP_METHOD_GET:
            return COAP_REQUEST_GET;

	case ATLAS_COAP_METHOD_POST:
            return COAP_REQUEST_POST;

	case ATLAS_COAP_METHOD_PUT:
            return COAP_REQUEST_PUT;

	case ATLAS_COAP_METHOD_DELETE:
            return COAP_REQUEST_DELETE;

        default:
	    return COAP_REQUEST_GET;
    }

    return COAP_REQUEST_GET;
}

static void
process_request_handler(coap_context_t *ctx,
                struct coap_resource_t *resource,
                coap_session_t *session,
                coap_pdu_t *request,
                coap_binary_t *token,
                coap_string_t *query,
                coap_pdu_t *response,
		atlas_coap_server_listener_t *listener)
{

    uint8_t *resp_payload = NULL;
    size_t resp_payload_len = 0;
    coap_string_t *coap_uri_path;
    char *uri_path;
    atlas_coap_response_t resp_code;
    uint8_t *req_payload = NULL;
    size_t req_payload_len = 0;

    /* Get request payload if any */
    coap_get_data(request, &req_payload_len, &req_payload);

    coap_uri_path = coap_get_uri_path(request);
    if (!coap_uri_path) {
        ATLAS_LOGGER_ERROR("Drop CoAP request: cannot get URI path!");
        return;
    }

    uri_path = (char*) calloc(sizeof(char), (coap_uri_path->length + 1));
    memcpy(uri_path, coap_uri_path->s, coap_uri_path->length);

    while(listener && listener->resource != resource)
        listener = listener->next;

    if (!listener) {
        ATLAS_LOGGER_ERROR("Drop CoAP request: there is no listener!");
        goto RET;
    }

    /* Add response code */
    resp_code = listener->callback(uri_path, req_payload, req_payload_len, &resp_payload, &resp_payload_len);
    response->code = COAP_RESPONSE_CODE(resp_code);
    
    /* Add payload if required */
    if (resp_code == ATLAS_COAP_RESP_OK && resp_payload && resp_payload_len)
        coap_add_data_blocked_response(resource, session, request, response, token,
                                       COAP_MEDIATYPE_TEXT_PLAIN, 0x2ffff,
                                       resp_payload_len, resp_payload);

    ATLAS_LOGGER_DEBUG("CoAP server response is sent");

RET:
    free(uri_path);
    free(resp_payload);
}


static void
get_handler(coap_context_t *ctx,
            struct coap_resource_t *resource,
            coap_session_t *session,
            coap_pdu_t *request,
            coap_binary_t *token,
            coap_string_t *query,
            coap_pdu_t *response)
{
    ATLAS_LOGGER_DEBUG("CoAP Server GET request");

    process_request_handler(ctx, resource, session, request,
                            token, query, response,
                            server_listeners[ATLAS_COAP_METHOD_GET]);

}

static void
post_handler(coap_context_t *ctx,
             struct coap_resource_t *resource,
             coap_session_t *session,
             coap_pdu_t *request,
             coap_binary_t *token,
             coap_string_t *query,
             coap_pdu_t *response)
{
    ATLAS_LOGGER_DEBUG("CoAP Server POST request");

    process_request_handler(ctx, resource, session, request,
                            token, query, response,
                            server_listeners[ATLAS_COAP_METHOD_POST]);

}

static void
put_handler(coap_context_t *ctx,
            struct coap_resource_t *resource,
            coap_session_t *session,
            coap_pdu_t *request,
            coap_binary_t *token,
            coap_string_t *query,
            coap_pdu_t *response)
{
    ATLAS_LOGGER_DEBUG("CoAP Server POST request");

    process_request_handler(ctx, resource, session, request,
                            token, query, response,
                            server_listeners[ATLAS_COAP_METHOD_PUT]);

}

static void
delete_handler(coap_context_t *ctx,
               struct coap_resource_t *resource,
               coap_session_t *session,
               coap_pdu_t *request,
               coap_binary_t *token,
               coap_string_t *query,
               coap_pdu_t *response)
{
    ATLAS_LOGGER_DEBUG("CoAP Server POST request");

    process_request_handler(ctx, resource, session, request,
                            token, query, response,
                            server_listeners[ATLAS_COAP_METHOD_DELETE]);

}


atlas_status_t
atlas_coap_server_start(uint16_t port, atlas_coap_server_mode_t server_mode, const char *psk)
{
    if (!port)
        return ATLAS_INVALID_PORT;

    coap_startup();
    
    ctx = get_context(port, server_mode, psk);
    if (!ctx) {
        ATLAS_LOGGER_ERROR("Cannot create CoAP context");
        return ATLAS_GENERAL_ERR;
    }
    
    init_default_resources(ctx);

    fd = coap_context_get_coap_fd(ctx);
    if (fd == -1) {
        ATLAS_LOGGER_INFO("Cannot get CoAP file descriptor");
        coap_free_context(ctx);
        ctx = NULL;

        return ATLAS_GENERAL_ERR;
    }

    /* Schedule CoAP server */
    atlas_sched_add_entry(fd, atlas_coap_server_sched_callback);

    return ATLAS_OK;
}

atlas_status_t
atlas_coap_server_add_resource(const char *uri_path, atlas_coap_method_t method, atlas_coap_server_cb_t cb)
{
    coap_resource_t *resource;
    atlas_coap_server_listener_t *listener, *p;

    if (!uri_path)
        return ATLAS_COAP_INVALID_URI;
    if (!cb)
        return ATLAS_INVALID_CALLBACK;

    resource = coap_resource_init(coap_make_str_const(uri_path), 0);
    if (!resource)
        return ATLAS_GENERAL_ERR;

    if (method == ATLAS_COAP_METHOD_GET)
        coap_register_handler(resource, atlas_coap_get_method(method), get_handler);
    else if (method == ATLAS_COAP_METHOD_POST)
        coap_register_handler(resource, atlas_coap_get_method(method), post_handler);
    else if (method == ATLAS_COAP_METHOD_PUT)
        coap_register_handler(resource, atlas_coap_get_method(method), put_handler);
    else if (method == ATLAS_COAP_METHOD_DELETE)
        coap_register_handler(resource, atlas_coap_get_method(method), delete_handler);

    coap_add_resource(ctx, resource);

    listener = (atlas_coap_server_listener_t *) malloc(sizeof(atlas_coap_server_listener_t));
    listener->resource = resource;
    listener->callback = cb;
    listener->uri_path = (char *) malloc(strlen(uri_path) + 1);
    strcpy(listener->uri_path, uri_path);
    listener->next = NULL;

    if (!server_listeners[method])
        server_listeners[method] = listener;
    else {
        p = server_listeners[method];
	while (p->next) p = p->next;

	p->next = listener;
    }

    return ATLAS_OK;
}

void
atlas_coap_server_del_resource(const char *uri_path, atlas_coap_method_t method)
{
    atlas_coap_server_listener_t *p, *pp;

    for (p = server_listeners[method]; p; p = p->next) {
        if (!strcmp(p->uri_path, uri_path)) {
	    if (p == server_listeners[method])
                server_listeners[method] = p->next;
	    else
                pp->next = p->next;

            ATLAS_LOGGER_DEBUG("Delete CoAP resource");
	    coap_delete_resource(ctx, p->resource); 
	    free(p->uri_path);
	    free(p);
	}
        pp = p;
    }
}
