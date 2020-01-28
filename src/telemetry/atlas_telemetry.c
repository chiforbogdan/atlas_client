#include <string.h>
#include <stdlib.h>
#include "atlas_telemetry.h"
#include "../logger/atlas_logger.h"
#include "../coap/atlas_coap_client.h"
#include "../register/atlas_register.h"
#include "../alarm/atlas_alarm.h"

#define ATLAS_CLIENT_TELEMETRY_FEATURE_TIMEOUT_MS (40000)

typedef struct _atlas_telemetry
{
    /* Telemetry feature URI */
    char uri[ATLAS_TELEMETRY_URI_LEN + 1];

    /* Telemetry callback for obtaining the feature payload */
    atlas_telemetry_payload_cb payload_cb;

    /* External push interval alarm id */
    atlas_alarm_id_t ext_push_id;

    /* Internal scan interval alarm id */
    atlas_alarm_id_t int_scan_id;

    /* Next element in the chain */
    struct _atlas_telemetry *next;
} atlas_telemetry_t;

/* Telemetry features */
static atlas_telemetry_t *features_;

static void atlas_telemetry_push(const atlas_telemetry_t *, uint8_t use_threshold);

static void
ext_push_alarm_cb(atlas_alarm_id_t alarm_id)
{
    ATLAS_LOGGER_DEBUG("Execute alarm callback for external push interval");
}
static void
int_scan_alarm_cb(atlas_alarm_id_t alarm_id)
{
    ATLAS_LOGGER_DEBUG("Execute alarm callback for internal scan interval");
}

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
            atlas_telemetry_push(p, ATLAS_TELEMETRY_SKIP_THRESHOLD);
            break;
        }
    }
}

static void
atlas_telemetry_push(const atlas_telemetry_t *feature, uint8_t use_threshold)
{
    uint8_t *payload = NULL;
    uint16_t payload_len = 0;
    atlas_status_t status;

    ATLAS_LOGGER_DEBUG("Pushing telemetry feature...");

    /* If the client is not registered to gateway, do not push the telemetry feature */
    if (!atlas_is_registered()) {
        ATLAS_LOGGER_ERROR("Client is not registered to gateway. Skipping feature...");
        return;
    }

    /* Get feature payload */
    feature->payload_cb(&payload, &payload_len, use_threshold);
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
    entry->ext_push_id = -1;
    entry->int_scan_id = -1;
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

void
atlas_telemetry_push_all()
{
    atlas_telemetry_t *p;

    ATLAS_LOGGER_INFO("Pushing all telemetry features to gateway...");

    for (p = features_; p; p = p->next)
        atlas_telemetry_push(p, ATLAS_TELEMETRY_SKIP_THRESHOLD);
}

void
atlas_telemetry_ext_push_set(const char *uri, uint16_t ext_push)
{
    atlas_telemetry_t *p;

    if (!uri)
        return;

    ATLAS_LOGGER_DEBUG("Set external push rate for telemetry feature");

    for (p = features_; p; p = p->next) {
        if (!strcmp(p->uri, uri)) {
            /* If external push value is 0, then push the feature right away */
            if (!ext_push) {
                atlas_telemetry_push(p, ATLAS_TELEMETRY_SKIP_THRESHOLD);
		break;
            }

	    /* Schedule alarm for external push */
            if (p->ext_push_id >= 0)
                atlas_alarm_cancel(p->ext_push_id);
	    
	    p->ext_push_id = atlas_alarm_set(ATLAS_ALARM_SEC_TO_MS(ext_push),
                                             ext_push_alarm_cb, ATLAS_ALARM_RUN_MULTIPLE_TIMES);
	    if (p->ext_push_id < 0)
                ATLAS_LOGGER_ERROR("Error when scheduling alarm for external push interval");

	    break;
        }
    }
}

void
atlas_telemetry_int_scan_set(const char *uri, uint16_t int_scan)
{
    atlas_telemetry_t *p;

    if (!uri || !int_scan)
        return;

    ATLAS_LOGGER_DEBUG("Set internal scan rate for telemetry feature");

    for (p = features_; p; p = p->next) {
        if (!strcmp(p->uri, uri)) {
	    /* Schedule alarm for internal scan */
            if (p->int_scan_id >= 0)
                atlas_alarm_cancel(p->int_scan_id);
	    
	    p->int_scan_id = atlas_alarm_set(ATLAS_ALARM_SEC_TO_MS(int_scan),
                                             int_scan_alarm_cb, ATLAS_ALARM_RUN_MULTIPLE_TIMES);
	    if (p->int_scan_id < 0)
                ATLAS_LOGGER_ERROR("Error when scheduling alarm for external push interval");

	    break;
        }
    }
}

