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

#define ATLAS_COAP_DTLS_NOT_SUPPORTED_ERR_STRING "CoAP DTLS is not supported"
#define ATLAS_COAP_DTLS_SRV_ERR_STRING "Cannot start CoAP server"

#define ATLAS_MAX_PSK_KEY_BYTES (64)

typedef struct _atlas_coap_server_listener
{
    coap_resource_t *resource;
    atlas_coap_server_cb_t callback;
    struct _atlas_coap_server_listener *next;
} atlas_coap_server_listener_t;

/* FIXME remove this*/
static uint8_t key[ATLAS_MAX_PSK_KEY_BYTES];
static ssize_t keyLen = 0;

/* CoAP server context */
static coap_context_t *ctx;
static int fd;

static atlas_coap_server_listener_t *server_listeners;

static int
set_dtls_psk(coap_context_t *ctx)
{
    coap_dtls_spsk_t psk;

    if (!coap_dtls_is_supported()) {
        printf("%s\n", ATLAS_COAP_DTLS_NOT_SUPPORTED_ERR_STRING);
        return ATLAS_COAP_SRV_DTLS_NOT_SUPPORTED;
    }

    memset(&psk, 0, sizeof(psk));
    psk.version = COAP_DTLS_SPSK_SETUP_VERSION;

    psk.psk_info.key.s = key;
    psk.psk_info.key.length = keyLen;

    /* Set PSK */
    coap_context_set_psk2(ctx, &psk);
}

static coap_context_t*
get_context(const char *hostname, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *res, *r;
    coap_context_t *ctx = NULL;
    int fd;
    coap_address_t addr, addrs;
    uint16_t tmp;
    coap_endpoint_t *ep, *eps;

    ctx = coap_new_context(NULL);
    if (!ctx)
        return NULL;

    /* Set DTLS PSK */
    set_dtls_psk(ctx);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    /* Use UDP */
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        printf("%s\n", ATLAS_COAP_DTLS_SRV_ERR_STRING);
        coap_free_context(ctx);
        return NULL;
    }

    for (r = res; r; r = r->ai_next) {
        if (r->ai_addrlen > sizeof(addr.addr))
            continue;
        
        coap_address_init(&addr);
        addr.size = r->ai_addrlen;
        memcpy(&addr.addr, r->ai_addr, addr.size);
        addrs = addr;

        if (addr.addr.sa.sa_family == AF_INET) {
            tmp = ntohs(addr.addr.sin.sin_port) + 1;
            addrs.addr.sin.sin_port = htons(tmp);
        } else if (addr.addr.sa.sa_family == AF_INET6) {
            uint16_t temp = ntohs(addr.addr.sin6.sin6_port) + 1;
            addrs.addr.sin6.sin6_port = htons(temp);
        }

        ep = coap_new_endpoint(ctx, &addr, COAP_PROTO_UDP);
        if (!ep) {
            printf("Cannot open COAP UDP\n");
            continue;
        }

        eps = coap_new_endpoint(ctx, &addrs, COAP_PROTO_DTLS);
        if (!eps)
            printf("Cannot open COAP DTLS\n");

        break;
    }

    if (!r) {
        printf("%s\n", ATLAS_COAP_DTLS_SRV_ERR_STRING);
        coap_free_context(ctx);
        return NULL;
    }

    freeaddrinfo(res);

    return ctx;
}

static void
get_default_index_handler(coap_context_t *ctx,
                  struct coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request,
                  coap_binary_t *token,
                  coap_string_t *query,
                  coap_pdu_t *response) {
 
    const char *INDEX = "Hello world ATLAS";

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
    
    coap_run_once(ctx, COAP_RUN_NONBLOCK);
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
    }

    return COAP_REQUEST_GET;
}

static void
get_index_handler(coap_context_t *ctx,
                  struct coap_resource_t *resource,
                  coap_session_t *session,
                  coap_pdu_t *request,
                  coap_binary_t *token,
                  coap_string_t *query,
                  coap_pdu_t *response)
{

    uint8_t *payload = NULL;
    uint16_t payload_len = 0;
    atlas_coap_server_listener_t *listener = server_listeners;
    coap_string_t *coap_uri_path;
    char *uri_path;

    coap_uri_path = coap_get_uri_path(request);
    if (!coap_uri_path) {
        ATLAS_LOGGER_ERROR("Drop CoAP request: cannot get URI path!");
        return;
    }

    uri_path = (char*) malloc(coap_uri_path->length + 1);
    memcpy(uri_path, coap_uri_path->s, coap_uri_path->length);

    while(listener && listener->resource != resource)
        listener = listener->next;

    if (!listener) {
        ATLAS_LOGGER_ERROR("Drop CoAP request: there is no listener!");
        goto RET;
    }

    listener->callback(uri_path, &payload, &payload_len);

    if (!payload || !payload_len) {
        ATLAS_LOGGER_ERROR("Drop CoAP request: invalid payload received from the application layer");
        goto RET;
    }

    coap_add_data_blocked_response(resource, session, request, response, token,
                                   COAP_MEDIATYPE_TEXT_PLAIN, 0x2ffff,
                                   payload_len, payload);

    ATLAS_LOGGER_DEBUG("CoAP server response is sent");

RET:
    free(uri_path);
    free(payload);
}

atlas_status_t
atlas_coap_server_start(const char *hostname, const char *port)
{
    if (!hostname)
        return ATLAS_INVALID_HOSTNAME;
    if (!port)
        return ATLAS_INVALID_PORT;

    coap_startup();
    
    ctx = get_context(hostname, port);
    
    init_default_resources(ctx);

    fd = coap_context_get_coap_fd(ctx);
    if (fd == -1) {
        ATLAS_LOGGER_INFO("Cannot get CoAP file descriptor");
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

    coap_register_handler(resource, atlas_coap_get_method(method), get_index_handler);
    coap_add_resource(ctx, resource);

    listener = (atlas_coap_server_listener_t *) malloc(sizeof(atlas_coap_server_listener_t));
    listener->resource = resource;
    listener->callback = cb;
    listener->next = NULL;

    if (!server_listeners)
        server_listeners = listener;
    else {
        p = server_listeners;
	while (p->next) p = p->next;

	p->next = listener;
    }
}
