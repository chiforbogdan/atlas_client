#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "atlas_identity.h"
#include "../logger/atlas_logger.h"

#define ATLAS_IDENTITY_FILE ".atlas_client_identity"
#define ATLAS_IDENTITY_PREFIX "IOT_CLIENT"
#define ATLAS_IDENTITY_LEN 16
#define ATLAS_PSK_LEN 32

static char identity[ATLAS_IDENTITY_LEN + 1];
static char psk[ATLAS_PSK_LEN + 1];

atlas_status_t
atlas_generate_identity()
{
    int fd;

    fd = open(ATLAS_IDENTITY_FILE, O_WRONLY);
    if (fd < 0) {
        ATLAS_LOGGER_DEBUG("Error when opening the identity file");
        return ATLAS_IDENTITY_FILE_ERROR;
    }

   /* Generate identity */
   
   /* Generate PSK */ 

    close(fd);

    return ATLAS_OK;
}

atlas_status_t
atlas_identity_init()
{
    int fd;

    fd = open(ATLAS_IDENTITY_FILE, O_RDONLY);
    if (fd < 0) {
        ATLAS_LOGGER_DEBUG("Identity file does not exist, generating an identity...");
        return atlas_generate_identity();
    }

    return ATLAS_OK;
}

const char *
atlas_identity_get()
{
    return identity;
}

const char *
atlas_psk_get()
{
    return psk;
}

