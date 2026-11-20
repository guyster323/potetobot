#ifndef POTETO_BLOB_H
#define POTETO_BLOB_H

#include <stddef.h>
#include <stdint.h>

int poteto_blob_find(const uint8_t *blob, size_t blob_len, const char *character,
                     const char *expression, int index, const uint8_t **payload, size_t *payload_len);

#endif
