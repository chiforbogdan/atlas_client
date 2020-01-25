#include <string.h>
#include <stdlib.h>
#include "atlas_telemetry.h"
#include "../logger/atlas_logger.h"
#include "../coap/atlas_coap_client.h"

#define ATLAS_CLIENT_TELEMETRY_FEATURE_TIMEOUT_MS (40000)

typedef struct _atlas_telemetry
{
    /* Telemetry feature URI */
    char uri[ATLAS_TELEMETRY_URI_LEN + 1];

    /* Telemetry callback for obtaining the feature payload */
    atlas_telemetry_payload_cb payload_cb;

    /* Next element in the chain */
    struct _atlas_telemetry *next;
} atlas_telemetry_t;

/* Telemetry features */
static atlas_telemetry_t *features_;

static void atlas_telemetry_push(const atlas_telemetry_t *);

static void
telemetry_callback(const char *uri, atlas_coap_response_t resp_status,
                   const uint8_t *resp_payload, size_t resp_payload_len)
{
    atlas_telemetry_t *p;

    ATLAS_LOGGER_DEBUG("Telemetry callback executed");

    /* If request succeeded, there is nothing to do */
    if (resp_status == ATLAS_COAP_RESP_OK) {
        ATLAS_LOGGER_DEBUG("Telemetry feature pushed successfully");
        return;
    }

    /* If gateway processed this request and returned an error, then skip it */
    if (resp_status != ATLAS_COAP_RESP_TIMEOUT) {
        ATLAS_LOGGER_INFO("Telemetry URI error on the gateway side. Abording request...");
        return;
    }

    /* Try to send the request again */
    for (p = features_; p; p = p->next) {
        if (!strcmp(p->uri, uri)) {
            atlas_telemetry_push(p);
            break;
        }
    }
}

static void
atlas_telemetry_push(const atlas_telemetry_t *feature)
{
    uint8_t *payload = NULL;
    uint16_t payload_len = 0;
    atlas_status_t status;

    ATLAS_LOGGER_DEBUG("Pushing telemetry feature...");

    /* Get feature payload */
    feature->payload_cb(&payload, &payload_len);
    if (!payload || !payload_len) {
        ATLAS_LOGGER_INFO("Feature payload empty. Skipping feature...");
        return;
    } 

    status = atlas_coap_client_request(feature->uri, ATLAS_COAP_METHOD_PUT,
                                       payload, payload_len,
                                       ATLAS_CLIENT_TELEMETRY_FEATURE_TIMEOUT_MS,
                                       telemetry_callback);

    free(payload);

    if (status != ATLAS_OK)
        ATLAS_LOGGER_ERROR("Error in sending telemetry feature to gateway");
}

void
atlas_telemetry_add(const char *uri, atlas_telemetry_payload_cb payload_cb)
{
    atlas_telemetry_t *entry, *p;

    if (!uri || !payload_cb)
        return;
    
    ATLAS_LOGGER_DEBUG("Add telemetry feature");

    entry = (atlas_telemetry_t *) malloc(sizeof(atlas_telemetry_t));
    strncpy(entry->uri, uri, sizeof(entry->uri) - 1);
    entry->payload_cb = payload_cb;
    entry->next = NULL;
    
    if (features_) {
        p = features_;
        while(p->next) p = p->next;
        
        p->next = entry;
    } else
        features_ = entry;
}
 
void
atlas_telemetry_del(const char *uri)
{
    atlas_telemetry_t *p, *pp;

    if (!uri)
        return;

    ATLAS_LOGGER_DEBUG("Remove telemetry feature");
    
    for (p = features_; p; p = p->next) {
        if (!strcmp(p->uri, uri)) {
            if (p == features_)
                features_ = p->next;
            else
                pp->next = p->next;
            
            free(p);
            break;
        }
        pp = p;
    }
}

void atlas_telemetry_push_all()
{
    atlas_telemetry_t *p;

    ATLAS_LOGGER_INFO("Pushing all telemetry features to gateway...");

    for (p = features_; p; p = p->next)
        atlas_telemetry_push(p);
}
