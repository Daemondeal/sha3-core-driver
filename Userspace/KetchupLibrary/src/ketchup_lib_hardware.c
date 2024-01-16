#include "../include/ketchup_lib.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <stdio.h>

#define KC_DEVICE_PATH "/dev/ketchup_driver"
#define WR_PERIPH_HASH_SIZE _IOW(0xFC, 1, uint32_t*)
#define RD_PERIPH_HASH_SIZE _IOR(0xFC, 2, uint32_t*)

#define KC_DIGEST_512 0
#define KC_DIGEST_384 1
#define KC_DIGEST_256 2
#define KC_DIGEST_224 3

static kc_error kc_init_peripheral(kc_sha3_context *context, uint8_t digest_length) {
    int fd = open(KC_DEVICE_PATH, O_RDWR);

    if (fd < 0) {
        if (fd == -EBUSY) {
            return KC_ERR_BUSY;
        }
        return KC_ERR_OTHER;
    }

    uint32_t dev_digest_setting = 0;
    // TODO: Set the peripheral to the appropriate size
    switch (digest_length) {
        case 512/8:
            dev_digest_setting = KC_DIGEST_512;
            break;
        case 384/8:
            dev_digest_setting = KC_DIGEST_384;
            break;
        case 256/8:
            dev_digest_setting = KC_DIGEST_256;
            break;
        case 224/8:
            dev_digest_setting = KC_DIGEST_224;
            break;
        default:
            return KC_ERR_UNSUPPORTED_SIZE;
    }

    int ioctl_retval = ioctl(fd, WR_PERIPH_HASH_SIZE, &dev_digest_setting);

    if (ioctl_retval != 0) {
        close(fd);
        return KC_ERR_UNSUPPORTED_SIZE;
    }

    context->fd = fd;
    context->digest_length = digest_length;

    return KC_ERR_NONE;
}


kc_error kc_sha3_512_init(kc_sha3_context *context) {
    return kc_init_peripheral(context, 512/8);
}

kc_error kc_sha3_384_init(kc_sha3_context *context) {
    return kc_init_peripheral(context, 384/8);
}

kc_error kc_sha3_256_init(kc_sha3_context *context) {
    return kc_init_peripheral(context, 256/8);
}

kc_error kc_sha3_224_init(kc_sha3_context *context) {
    return kc_init_peripheral(context, 224/8);
}

void kc_sha3_update(kc_sha3_context *context, const void *new_data, uint32_t new_data_length) {
    ssize_t data_written;
    size_t remaining_data = new_data_length;
    void const *data_ptr = new_data;

    while (remaining_data > 0) {
        data_written = write(context->fd, data_ptr, remaining_data);
        if (data_written < 0) {
            // Something bad happened
            // TODO: Check if this is actually realistic, 
            //       and if it is change the function signature
            return;
        }

        remaining_data -= data_written;
        data_ptr += data_written;
    }
}

void kc_sha3_final(kc_sha3_context *context, uint8_t *digest, uint32_t *digest_length) {
    ssize_t remaining_data = context->digest_length;
    ssize_t data_read;

    uint8_t *digest_ptr = digest;

    while (remaining_data > 0) {
        data_read = read(context->fd, digest_ptr, remaining_data);
        if (data_read < 0) {
            // Something bad happened
            // TODO: Check if this is actually realistic, 
            //       and if it is change the function signature
            return;
        }
        remaining_data -= data_read;
        digest_ptr += data_read;
    }

    *digest_length = context->digest_length;
}

kc_error kc_sha3_close(kc_sha3_context *context) {
    int error = close(context->fd);
    if (error < 0) {
        return KC_ERR_OTHER;
    }

    return KC_ERR_NONE;
}
