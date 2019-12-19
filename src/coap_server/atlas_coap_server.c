#include "atlas_coap_server.h"
#include <stdio.h>
#include <coap2/coap.h> 
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define ATLAS_COAP_DTLS_NOT_SUPPORTED_ERR_STRING "CoAP DTLS is not supported"
#define ATLAS_COAP_DTLS_SRV_ERR_STRING "Cannot start CoAP server"

#define ATLAS_MAX_PSK_KEY_BYTES (64)
/* FIXME remove this*/
static uint8_t key[ATLAS_MAX_PSK_KEY_BYTES];
static ssize_t keyLen = 0;

/* CoAP server context */
static coap_context_t *ctx;

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
get_index_handler(coap_context_t *ctx,
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
init_resources(coap_context_t *ctx)
{
    coap_resource_t *resource;

    resource = coap_resource_init(NULL, 0);
    coap_register_handler(resource, COAP_REQUEST_GET, get_index_handler);

    coap_add_resource(ctx, resource);
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
    
    init_resources(ctx);

    return ATLAS_OK;
}

void
atlas_coap_server_loop()
{
    int fd, nfds;
    fd_set readfds;
    int result;

    fd = coap_context_get_coap_fd(ctx);
    if (fd == -1) {
        printf("Cannot get COAP FD\n");
        return;
    }

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    nfds = fd + 1;
    
    while (1) {
        result = select(nfds, &readfds, NULL, NULL, NULL);
        if (result > 0 && FD_ISSET(fd, &readfds)) {
            printf("Server CoAP request...\n");
            coap_run_once(ctx, COAP_RUN_NONBLOCK);
        }
    }
}

