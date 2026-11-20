#include "blob.h"

#include <string.h>

int poteto_blob_find(const uint8_t *blob, size_t blob_len, const char *character,
                     const char *expression, int index, const uint8_t **payload, size_t *payload_len) {
    size_t offset;
    unsigned int loops;
    unsigned int n;

    if (blob == NULL || character == NULL || expression == NULL || payload == NULL || payload_len == NULL) {
        return -1;
    }
    if (blob_len < 8 || blob[0] != 'P' || blob[1] != 'T' || blob[2] != 'R' || blob[3] != '1') {
        return -1;
    }
    loops = (unsigned int)blob[4] | ((unsigned int)blob[5] << 8);
    offset = 8;
    for (n = 0; n < loops; n++) {
        unsigned int count;
        unsigned int frame;
        if (offset + 36 > blob_len) {
            return -1;
        }
        count = (unsigned int)blob[offset + 32] | ((unsigned int)blob[offset + 33] << 8);
        if (strcmp((const char *)(blob + offset), character) == 0 &&
            strcmp((const char *)(blob + offset + 16), expression) == 0) {
            if (index < 0 || (unsigned int)index >= count) {
                return -1;
            }
            frame = offset + 36 + (unsigned int)index * 8;
            if (frame + 8 > blob_len) {
                return -1;
            }
            {
                unsigned int data_off = (unsigned int)blob[frame] | ((unsigned int)blob[frame + 1] << 8) |
                                        ((unsigned int)blob[frame + 2] << 16) | ((unsigned int)blob[frame + 3] << 24);
                unsigned int data_len = (unsigned int)blob[frame + 4] | ((unsigned int)blob[frame + 5] << 8) |
                                        ((unsigned int)blob[frame + 6] << 16) | ((unsigned int)blob[frame + 7] << 24);
                if ((size_t)data_off + data_len > blob_len) {
                    return -1;
                }
                *payload = blob + data_off;
                *payload_len = data_len;
                return 0;
            }
        }
        if (offset + 36 + (size_t)count * 8 > blob_len) {
            return -1;
        }
        offset += 36 + (size_t)count * 8;
    }
    return -1;
}
