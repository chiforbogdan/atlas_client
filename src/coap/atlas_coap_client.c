#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <coap2/coap.h>
#include "atlas_coap_client.h"
#include "../logger/atlas_logger.h"
#include "../scheduler/atlas_scheduler.h"
#include "../alarm/atlas_alarm.h"

#define ATLAS_COAP_CLIENT_DTLS_IDENTITY_LEN (64)
#define ATLAS_COAP_CLIENT_DTLS_KEY_LEN (64)

#define ATLAS_COAP_CLIENT_TMP_BUF_LEN (128)

typedef struct _atlas_coap_client_req
{
    /* CoAP request file descriptor */
    int coap_fd;

    /* Timeout alarm id */
    atlas_alarm_id_t alarm_id;

    /* CoAP context */
    coap_context_t *ctx;

    /* CoAP session */
    coap_session_t *session;

    /* CoAP request token */
    uint32_t token;

    /* Higher layer application callback */
    atlas_coap_client_cb_t callback;

    /* Indicates if the entry is dirty and should be deleted */
    uint8_t dirty;

    /* Next entry */
    struct _atlas_coap_client_req *next;

} atlas_coap_client_req_t;

static atlas_coap_client_req_t *req_entry;
static coap_dtls_cpsk_t dtls_psk;
static char dtls_identity[ATLAS_COAP_CLIENT_DTLS_IDENTITY_LEN + 1];
static char dtls_key[ATLAS_COAP_CLIENT_DTLS_KEY_LEN + 1];

static uint32_t
get_new_token()
{
    static uint32_t token = 0;

    return token++;
}

static int
resolve_address(coap_str_const_t *hostname, struct sockaddr *dst)
{
    struct addrinfo *res, *ainfo;
    struct addrinfo hints;
    char *addr;
    int status;
    int len;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    addr = (char *) malloc(hostname->length + 1);
    memset(addr, 0, hostname->length + 1);
    memcpy(addr, hostname->s, hostname->length);

    status = getaddrinfo(addr, NULL, &hints, &res);
    
    free(addr);

    if (status) {
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
    if (proto == COAP_PROTO_DTLS)
        return coap_new_client_session_psk2(ctx, NULL, dst, proto, &dtls_psk);

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


static int
validate_token(coap_pdu_t *received, uint32_t token)
{
    if (received->token_length != sizeof(token))
        return 0;

    return memcmp(received->token, (uint8_t*) &token, sizeof(token)) == 0;
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
    atlas_coap_client_req_t *ent;

    ATLAS_LOGGER_INFO("CoAP client message handler");

    /* Find request entry */
    for (ent = req_entry; ent; ent = ent->next) {
        if (ent->ctx == ctx && ent->session == session)
            break;
    }

    if (!ent) {
        ATLAS_LOGGER_ERROR("CoAP client: cannot find request entry associated with context and session");
        return;
    }
    
    /* Validate token */
    if (!validate_token(received, ent->token)) {
        ATLAS_LOGGER_INFO("CoAP client: received CoAP response with invalid token");

        if ((received->type == COAP_MESSAGE_CON || received->type == COAP_MESSAGE_NON) &&
            !sent)
            coap_send_rst(session, received);
        
	return;
    }

    /* If token is valid, then the request entry can be deleted */
    ent->dirty = 1;

    if (received->type == COAP_MESSAGE_RST) {
        ATLAS_LOGGER_INFO("CoAP client: got RST as response");
        ent->callback(ATLAS_COAP_RESP_RESET, NULL, 0);
	return;
    }

    /* If response is SUCCESS */
    if (COAP_RESPONSE_CLASS(received->code) == 2) {
        ATLAS_LOGGER_DEBUG("CoAP client: Response code is 200");
        /* Get response payload, if any */
        coap_get_data(received, &resp_payload_len, &resp_payload);

	/* Call the higher layer application callback */
        ent->callback(ATLAS_COAP_RESP_OK, resp_payload, resp_payload_len);
    } else if (COAP_RESPONSE_CLASS(received->code) == 4) {
        ATLAS_LOGGER_DEBUG("CoAP client: Response code is 4XX");
        ent->callback(ATLAS_COAP_RESP_NOT_FOUND, NULL, 0);
    } else {
        ATLAS_LOGGER_DEBUG("CoAP client: Response code is UNKNOWN");
        ent->callback(ATLAS_COAP_RESP_UNKNOWN, NULL, 0);
    }

}

static void
coap_client_sched_callback(int fd)
{
    atlas_coap_client_req_t *p, *pp;
    
    ATLAS_LOGGER_DEBUG("CoAP client: Handling CoAP response...");

    for (p = req_entry; p; p = p->next) {
        if (p->coap_fd == fd) {

            coap_run_once(p->ctx, COAP_RUN_NONBLOCK);

            /* If entry is dirty, then it can be deleted */
	    if (p->dirty) {    
                if (p == req_entry)
                    req_entry = req_entry->next;
                else
                    pp->next = p->next;

                atlas_sched_del_entry(p->coap_fd);
	        atlas_alarm_cancel(p->alarm_id);
		coap_session_release(p->session);
	        coap_free_context(p->ctx);
	        free(p);
	    }
	    break;
	}
        pp = p;
    }	
}

static void request_alarm_cb(atlas_alarm_id_t alarm_id)
{
    atlas_coap_client_req_t *p, *pp;
    
    ATLAS_LOGGER_DEBUG("CoAP client: Handling response timeout");

    for (p = req_entry; p; p = p->next) {
        if (p->alarm_id == alarm_id) {
            p->callback(ATLAS_COAP_RESP_TIMEOUT, NULL, 0);

            if (p == req_entry)
                req_entry = req_entry->next;
            else
                pp->next = p->next;

            atlas_sched_del_entry(p->coap_fd);
            coap_session_release(p->session);
            coap_free_context(p->ctx);
            free(p);

            break;
        }
        pp = p;
    }
}

static atlas_status_t
add_request(coap_context_t *ctx, coap_session_t *session, uint32_t token,
            uint16_t timeout, atlas_coap_client_cb_t callback)
{
    int fd;
    atlas_coap_client_req_t *entry, *p;
    atlas_status_t status;

    entry = (atlas_coap_client_req_t*) malloc(sizeof(atlas_coap_client_req_t));
    entry->ctx = ctx;
    entry->session = session;
    entry->token = token;
    entry->callback = callback;
    entry->dirty = 0;
    entry->next = NULL;

    /* Get CoAP request file descriptor */
    fd = coap_context_get_coap_fd(ctx);
    if (fd == -1) {
        ATLAS_LOGGER_ERROR("Cannot get CoAP file descriptor");
        status = ATLAS_GENERAL_ERR;
	goto ERR;
    }
    entry->coap_fd = fd;

    /* Set alarm for the CoAP request */
    entry->alarm_id = atlas_alarm_set(timeout, request_alarm_cb, 1);
    if (entry->alarm_id < 0) {
        ATLAS_LOGGER_ERROR("Cannot set alarm for CoAP client request");
	status = ATLAS_GENERAL_ERR;
	goto ERR;
    }

    /* Schedule CoAP response for this request */
    atlas_sched_add_entry(fd, coap_client_sched_callback); 

    if (!req_entry)
        req_entry = entry;
    else {
        p = req_entry;
	while(p->next) p = p->next;

	p->next = entry;
    }

    return ATLAS_OK; 
ERR:
    free(entry);
    
    return status;
}

atlas_status_t
atlas_coap_client_request(const char *uri, atlas_coap_method_t method,
                          const uint8_t *req_payload, size_t req_payload_len,
                          uint16_t timeout, atlas_coap_client_cb_t cb)
{
    coap_address_t dst;
    coap_context_t *ctx = NULL;
    coap_session_t *session = NULL;
    coap_pdu_t *req_pdu = NULL;
    coap_optlist_t *options = NULL;
    coap_uri_t coap_uri;
    coap_str_const_t hostname;
    int res;
    uint32_t token;
    uint8_t buf[ATLAS_COAP_CLIENT_TMP_BUF_LEN];
    uint8_t *buf_tmp;
    size_t buflen;
    atlas_status_t status = ATLAS_OK;

    if (!uri)
        return ATLAS_COAP_INVALID_URI;
    if (!cb)
        return ATLAS_INVALID_CALLBACK;

    if (coap_split_uri((uint8_t *)uri, strlen(uri), &coap_uri) < 0) {
        ATLAS_LOGGER_ERROR("Invalid CoAP URI");
        return ATLAS_GENERAL_ERR;
    }

    /* Resolve CoAP hostname */
    hostname = coap_uri.host;
    res = resolve_address(&hostname, &dst.addr.sa);
    if (res < 0)
        return ATLAS_GENERAL_ERR;    

    /* Context init */
    ctx = coap_new_context(NULL);
    if (!ctx) {
        ATLAS_LOGGER_ERROR("Cannot create CoAP context for client request");
        return ATLAS_GENERAL_ERR;
    }

    /* Register handlers */
    coap_register_response_handler(ctx, message_handler);
    coap_register_event_handler(ctx, event_handler);
    coap_register_nack_handler(ctx, nack_handler);

    dst.size = res;
    dst.addr.sin.sin_port = htons(coap_uri.port);

    /* Get session */
    if (coap_uri.scheme == COAP_URI_SCHEME_COAPS)
        session = get_session(ctx, COAP_PROTO_DTLS, &dst);
    else
        session = get_session(ctx, COAP_PROTO_UDP, &dst);

    if (!session) {
        ATLAS_LOGGER_ERROR("Cannot create CoAP session for client request");
        status = ATLAS_GENERAL_ERR;
        goto ERR;
    }

    /* Create request PDU */
    if (!(req_pdu = coap_new_pdu(session))) {
        ATLAS_LOGGER_ERROR("Cannot create client request PDU");
        status = ATLAS_GENERAL_ERR;
        goto ERR;
    }

    req_pdu->type = COAP_MESSAGE_CON;
    req_pdu->tid = coap_new_message_id(session);
    
    switch(method) {
        case ATLAS_COAP_METHOD_GET:
            req_pdu->code = COAP_REQUEST_GET;
            break;

        case ATLAS_COAP_METHOD_POST:
            req_pdu->code = COAP_REQUEST_POST;
            break;

        case ATLAS_COAP_METHOD_PUT:
            req_pdu->code = COAP_REQUEST_PUT;
            break;

        case ATLAS_COAP_METHOD_DELETE:
            req_pdu->code = COAP_REQUEST_DELETE;
            break;

        default:
            req_pdu->code = COAP_REQUEST_GET;
    }
 
    /* Add token */
    token = get_new_token();
    if (!coap_add_token(req_pdu, sizeof(uint32_t), (uint8_t*) &token)) {
        ATLAS_LOGGER_ERROR("Cannot add token to CoAP client request");
        status = ATLAS_GENERAL_ERR;
        goto ERR;
    }

    /* Add URI path */
    if (coap_uri.path.length) {
        buflen = sizeof(buf);
        buf_tmp = buf;
        res = coap_split_path(coap_uri.path.s, coap_uri.path.length, buf_tmp, &buflen);

        while (res--) {
            coap_insert_optlist(&options,
                                coap_new_optlist(COAP_OPTION_URI_PATH,
                                coap_opt_length(buf_tmp),
                                coap_opt_value(buf_tmp)));
 
            buf_tmp += coap_opt_size(buf_tmp);
        }
    }

    /* Add URI query */
    if (coap_uri.query.length) {
        buflen = sizeof(buf);
        buf_tmp = buf;
        res = coap_split_query(coap_uri.query.s, coap_uri.query.length, buf_tmp, &buflen);
 
        while (res--) {
            coap_insert_optlist(&options,
                                coap_new_optlist(COAP_OPTION_URI_QUERY,
                                coap_opt_length(buf_tmp),
                                coap_opt_value(buf_tmp)));
 
            buf_tmp += coap_opt_size(buf_tmp);
        }
    }

    if (options) {
        coap_add_optlist_pdu(req_pdu, &options);
        coap_delete_optlist(options);
    }

    /* Add request payload */
    if (req_payload && req_payload_len)
        coap_add_data(req_pdu, req_payload_len, req_payload);

    /* Chain request */
    status = add_request(ctx, session, token, timeout, cb);
    if (status != ATLAS_OK) {
        ATLAS_LOGGER_ERROR("Cannot add the CoAP request to the request chain");
        goto ERR;
    }

    /* Send request */
    ATLAS_LOGGER_DEBUG("Sending CoAP client request...");
    coap_send(session, req_pdu);

    return ATLAS_OK;

ERR:
    coap_session_release(session);
    coap_free_context(ctx);

    return status;
}

atlas_status_t
atlas_coap_client_set_dtls_info(const char *identity, const char *psk)
{
    if (!identity)
        return ATLAS_INVALID_DTLS_IDENTITY;
    if (!psk)
        return ATLAS_INVALID_DTLS_PSK;

    memset (&dtls_psk, 0, sizeof(dtls_psk));
    dtls_psk.version = COAP_DTLS_CPSK_SETUP_VERSION;

    /* Set DTLS identity */
    strncpy(dtls_identity, identity, sizeof(dtls_identity) - 1);
    dtls_psk.psk_info.identity.s = (uint8_t *) dtls_identity;
    dtls_psk.psk_info.identity.length = strlen(dtls_identity);

    /* Set DTLS PSK */
    strncpy(dtls_key, psk, sizeof(dtls_key) - 1);
    dtls_psk.psk_info.key.s = (uint8_t *) dtls_key;
    dtls_psk.psk_info.key.length = strlen(dtls_key);

    return ATLAS_OK;
}
