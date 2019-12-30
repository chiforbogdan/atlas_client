#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <coap2/coap.h>
#include "atlas_coap_client.h"
#include "../logger/atlas_logger.h"
#include "../scheduler/atlas_scheduler.h"

#define ATLAS_COAP_CLIENT_TOKEN_LEN (4)

static coap_context_t *ctx;
static uint8_t token[ATLAS_COAP_CLIENT_TOKEN_LEN];

static int
resolve_address(const char *uri, struct sockaddr *dst)
{
    struct addrinfo *res, *ainfo;
    struct addrinfo hints;
    int len;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    if (getaddrinfo(uri, NULL, &hints, &res)) {
        ATLAS_LOGGER_ERROR("Cannot resolve CoAP request address");
        return -1;
    }

    for (ainfo = res; ainfo; ainfo = ainfo->ai_next) {
        if (ainfo->ai_family == AF_INET || ainfo->ai_family == AF_INET6) {
            memcpy(dst, ainfo->ai_addr, ainfo->ai_addrlen);
            len = ainfo->ai_addrlen; 
            break;
        }
    }

    freeaddrinfo(res);

    return len;
}

static coap_session_t*
get_session(coap_context_t *ctx, coap_proto_t proto, coap_address_t *dst)
{
    /* FIXME add DTLS support */
    return coap_new_client_session(ctx, NULL, dst, proto);
}

static int
event_handler(coap_context_t *ctx, coap_event_t event,
              struct coap_session_t *session)
{
    ATLAS_LOGGER_INFO("CoAP client event handler");

    switch(event) {
        case COAP_EVENT_DTLS_CLOSED:
            ATLAS_LOGGER_INFO("CoAP client request event: DTLS CLOSED");
            break;
        case COAP_EVENT_TCP_CLOSED:
            ATLAS_LOGGER_INFO("CoAP client request event: TCP CLOSED");
            break;
        case COAP_EVENT_SESSION_CLOSED:
            ATLAS_LOGGER_INFO("CoAP client request event: SESSION CLOSED");
            break;
        default:
            break;
    }

     return 0;
}

static void
nack_handler(coap_context_t *context, coap_session_t *session,
             coap_pdu_t *sent, coap_nack_reason_t reason,
             const coap_tid_t id) {
 
    ATLAS_LOGGER_INFO("CoAP client nack handler");
    
    switch(reason) {
        case COAP_NACK_TOO_MANY_RETRIES:
            ATLAS_LOGGER_INFO("Coap client NACK: TOO MANY RETRIES");
            break;
        case COAP_NACK_NOT_DELIVERABLE:
            ATLAS_LOGGER_INFO("Coap client NACK: NOT DELIVERABLE");
            break;
        case COAP_NACK_RST:
            ATLAS_LOGGER_INFO("Coap client NACK: RST");
            break;
        case COAP_NACK_TLS_FAILED:
            ATLAS_LOGGER_INFO("Coap client NACK: TLS FAILED");
            break;
        case COAP_NACK_ICMP_ISSUE:
            ATLAS_LOGGER_INFO("Coap client NACK: ICMP ISSUE");
            break;
        default:
             break;
    }
 }


static void
message_handler(struct coap_context_t *ctx,
                coap_session_t *session,
                coap_pdu_t *sent,
                coap_pdu_t *received,
                const coap_tid_t id)
{
    uint8_t *resp_payload = NULL;
    size_t resp_payload_len = 0;
    int i;

    ATLAS_LOGGER_INFO("CoAP client message handler");

    /* FIXME check token */
    if (received->type == COAP_MESSAGE_RST) {
        ATLAS_LOGGER_INFO("CoAP client: got RST as response");
        return;
    }

    /* If response is SUCCESS */
    if (COAP_RESPONSE_CLASS(received->code) == 2) {
        /* Returns 0 on error*/
        if (!coap_get_data(received, &resp_payload_len, &resp_payload)) {
            ATLAS_LOGGER_ERROR("CoAP client: cannot get CoAP response payload");
            return;
        }
        for (i = 0; i < resp_payload_len; i++)
            printf("%c", resp_payload[i]);

        printf("\n");
    }
}

static void
atlas_coap_client_sched_callback(int fd)
{
    ATLAS_LOGGER_DEBUG("CoAP client: Handling CoAP response...");

    coap_run_once(ctx, COAP_RUN_NONBLOCK);
}

atlas_status_t
atlas_coap_client_request(const char *uri, uint16_t port, const uint8_t *req_payload, size_t req_payload_len, atlas_coap_client_cb_t cb)
{
    coap_address_t dst;
    coap_session_t *session = NULL;
    coap_pdu_t *req_pdu = NULL;
    int res;
    int fd;
    atlas_status_t status = ATLAS_OK;

    if (!uri)
        return ATLAS_COAP_INVALID_URI;
    if (!port)
        return ATLAS_INVALID_PORT;    
    if (!cb)
        return ATLAS_INVALID_CALLBACK;

    /* Resolve CoAP URI */
    res = resolve_address(uri, &dst.addr.sa);
    if (res < 0)
        return ATLAS_GENERAL_ERR;    

    /* Lazy init for context */
    if (!ctx) {
        ctx = coap_new_context(NULL);
        if (!ctx) {
            ATLAS_LOGGER_ERROR("Cannot create CoAP context for client request");
            return ATLAS_GENERAL_ERR;
        }

        /* Register handlers */
        coap_register_option(ctx, COAP_OPTION_BLOCK2);
        coap_register_response_handler(ctx, message_handler);
        coap_register_event_handler(ctx, event_handler);
        coap_register_nack_handler(ctx, nack_handler);
    }

    dst.size = res;
    dst.addr.sin.sin_port = htons(port);

    /* Get session */
    session = get_session(ctx, COAP_PROTO_UDP, &dst);
    if (!session) {
        ATLAS_LOGGER_ERROR("Cannot create CoAP session for client request");
        return ATLAS_GENERAL_ERR;
    }

    /* Create request PDU */
    if (!(req_pdu = coap_new_pdu(session))) {
        ATLAS_LOGGER_ERROR("Cannot create client request PDU");
        status = ATLAS_GENERAL_ERR;
        goto ERROR;
    }
 
    req_pdu->type = COAP_REQUEST_GET;
    req_pdu->tid = coap_new_message_id(session);
    req_pdu->code = COAP_REQUEST_GET;
 
    if (!coap_add_token(req_pdu, ATLAS_COAP_CLIENT_TOKEN_LEN, token)) {
        ATLAS_LOGGER_ERROR("Cannot add token to CoAP client request");
        status = ATLAS_GENERAL_ERR;
        goto ERROR;
    }
  
    /* Add request payload */
    if (req_payload && req_payload_len)
        coap_add_data(req_pdu, req_payload_len, req_payload);

    /* Send CoAP request */
    ATLAS_LOGGER_DEBUG("Sending CoAP client request...");

    fd = coap_context_get_coap_fd(ctx);
    if (fd == -1) {
        ATLAS_LOGGER_ERROR("Cannot get CoAP file descriptor");
        status = ATLAS_GENERAL_ERR;
        goto ERROR;
    }

    /* Schedule CoAP server */
    atlas_sched_add_entry(fd, atlas_coap_client_sched_callback);

    coap_send(session, req_pdu);
ERROR:
    //coap_session_release(session);

    return status;
}
