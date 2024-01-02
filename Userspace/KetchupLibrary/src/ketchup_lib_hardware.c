#include "./ketchup_lib.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define KC_DEVICE_PATH "/dev/ketchup"

static kc_error kc_init_peripheral(kc_sha3_context *context, uint8_t digest_length) {
    int fd = open(KC_DEVICE_PATH, O_RDWR);

    if (fd < 0) {
        if (fd == -EBUSY) {
            return KC_ERR_BUSY;
        }
        return KC_ERR_OTHER;
    }

    // TODO: Set the peripheral to the appropriate size
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
    size_t remaining_data = context->digest_length;
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
}

kc_error kc_sha3_close(kc_sha3_context *context) {
    int error = close(context->fd);
    if (error < 0) {
        return KC_ERR_OTHER;
    }

    return KC_ERR_NONE;
}
