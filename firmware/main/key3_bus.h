#ifndef POTETO_KEY3_BUS_H
#define POTETO_KEY3_BUS_H

#include <stdint.h>

typedef struct poteto_bus_io {
    void (*set_sda)(void *ctx, int high);
    void (*set_scl)(void *ctx, int high);
    int (*read_sda)(void *ctx);
    int (*read_scl)(void *ctx);
    void (*delay)(void *ctx);
    void *ctx;
} poteto_bus_io;

typedef enum poteto_bus_status {
    POTETO_BUS_ACK = 0,
    POTETO_BUS_NACK = 1,
    POTETO_BUS_SDA_STUCK = 2,
    POTETO_BUS_SCL_STUCK = 3
} poteto_bus_status;

/* Start, write each byte, stop. Stuck SDA or SCL is not an ACK.
   Address NACK is not an ACK. A slave that pulls SDA only on the
   ack clock returns POTETO_BUS_ACK. */
poteto_bus_status poteto_bus_txn(const poteto_bus_io *io, const uint8_t *bytes, int count);

/* Write addr and reg, repeated start, read one byte. A stuck clock or a
   NACK on either address is not a successful read. */
poteto_bus_status poteto_bus_read_reg(const poteto_bus_io *io, uint8_t addr_w, uint8_t reg, uint8_t addr_r,
                                      uint8_t *value);

/* Opened codec uses the first string. Every bus fault uses the second. */
const char *poteto_key3_caption(int opened);

#endif
