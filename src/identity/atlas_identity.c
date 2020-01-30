#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include "atlas_identity.h"
#include "../logger/atlas_logger.h"

#define ATLAS_IDENTITY_FILE "atlas_client.identity"
#define ATLAS_PSK_FILE      "atlas_client.psk"
#define ATLAS_IDENTITY_PREFIX "IOT-CLIENT-"
#define ATLAS_IDENTITY_LEN 64
#define ATLAS_PSK_LEN 64

static char identity[ATLAS_IDENTITY_LEN + 1];
static char psk[ATLAS_PSK_LEN + 1];

static atlas_status_t
atlas_generate_identity()
{
    int fd;
    uuid_t uuid;

    ATLAS_LOGGER_DEBUG("Generate identity...");

    fd = open(ATLAS_IDENTITY_FILE, O_WRONLY | O_CREAT | O_TRUNC,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        ATLAS_LOGGER_DEBUG("Error when opening the identity file");
        return ATLAS_IDENTITY_FILE_ERROR;
    }

   /* Generate identity */
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, identity);
    write(fd, identity, strlen(identity));

    close(fd);

    ATLAS_LOGGER_DEBUG("Identity was generated");

    return ATLAS_OK;
}

static void
to_base64(const uint8_t *in, size_t in_len, char *out, size_t out_len)
{
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, in, in_len);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    memcpy(out, bptr->data,
           out_len > bptr->length ? bptr->length : out_len);

    BIO_free_all(b64);
}


static atlas_status_t
atlas_generate_psk()
{
    int fd;
    uint8_t rand_bytes[ATLAS_PSK_LEN/2];

    ATLAS_LOGGER_DEBUG("Generate PSK...");

    fd = open(ATLAS_PSK_FILE, O_WRONLY | O_CREAT | O_TRUNC,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        ATLAS_LOGGER_DEBUG("Error when opening the PSK file");
        return ATLAS_PSK_FILE_ERROR;
    }

    /* Generate PSK */
    if (RAND_bytes(rand_bytes, sizeof(rand_bytes)) != 1) {
        close(fd);
	return ATLAS_PSK_FILE_ERROR;
    }
    memset(psk, 0, sizeof(psk));
    to_base64(rand_bytes, sizeof(rand_bytes), psk, sizeof(psk) - 1);
    write(fd, psk, sizeof(psk));

    close(fd);

    return ATLAS_OK;
}

atlas_status_t
atlas_identity_init()
{
    atlas_status_t status = ATLAS_GENERAL_ERR;
    int fd;
    int len;

    ATLAS_LOGGER_DEBUG("Identity init...");

    /* Get identity */
    fd = open(ATLAS_IDENTITY_FILE, O_RDONLY);
    if (fd < 0) {
        ATLAS_LOGGER_DEBUG("Identity file does not exist, generating an identity...");
        status = atlas_generate_identity();
	if (status != ATLAS_OK) {
            status = ATLAS_IDENTITY_FILE_ERROR;
            goto ERR;
	}
    } else {
        memset(identity, 0, sizeof(identity));
        len = read(fd, identity, sizeof(identity) - 1);
        close (fd);
        if (len <= 0) {
            ATLAS_LOGGER_ERROR("Error encountered when reading from the identity file");
            status = ATLAS_IDENTITY_FILE_ERROR;
            goto ERR;
        }
    }

    /* Get PSK */
    fd = open(ATLAS_PSK_FILE, O_RDONLY);
    if (fd < 0) {
        ATLAS_LOGGER_DEBUG("PSK file does not exist, generating a PSK...");
        status = atlas_generate_psk();
        if (status != ATLAS_OK) {
	    status = ATLAS_PSK_FILE_ERROR;
            goto ERR;
	}	
    } else {
        memset(psk, 0, sizeof(psk));
        len = read(fd, psk, sizeof(psk) - 1);
        close(fd);
        if (len <= 0) {
            ATLAS_LOGGER_ERROR("Error encountered when reading from the PSK file");
            status = ATLAS_PSK_FILE_ERROR;
            goto ERR;
        }
    }

    return ATLAS_OK;

ERR:
    remove(ATLAS_IDENTITY_FILE);
    remove(ATLAS_PSK_FILE);

    return status;
}

const char *
atlas_identity_get()
{
    //return identity;
    /* FIXME Temporary workaround until the gateway supports multiple identities */
    return "test1";
}

const char *
atlas_psk_get()
{
    //return psk;
    /* FIXME Temporary workaround until the gateway supports multiple identities */
    return "12345678";
}

