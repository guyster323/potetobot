#include "key3_bus.h"

#include <stddef.h>

static void bus_delay(const poteto_bus_io *io) {
    if (io->delay != NULL) {
        io->delay(io->ctx);
    }
}

static poteto_bus_status scl_release(const poteto_bus_io *io) {
    int n;
    io->set_scl(io->ctx, 1);
    for (n = 0; n < 40; n++) {
        if (io->read_scl(io->ctx)) {
            return POTETO_BUS_ACK;
        }
        bus_delay(io);
    }
    return POTETO_BUS_SCL_STUCK;
}

static poteto_bus_status bus_idle(const poteto_bus_io *io) {
    io->set_sda(io->ctx, 1);
    io->set_scl(io->ctx, 1);
    if (!io->read_scl(io->ctx)) {
        return POTETO_BUS_SCL_STUCK;
    }
    if (!io->read_sda(io->ctx)) {
        return POTETO_BUS_SDA_STUCK;
    }
    return POTETO_BUS_ACK;
}

static void bus_start(const poteto_bus_io *io) {
    io->set_sda(io->ctx, 1);
    io->set_scl(io->ctx, 1);
    bus_delay(io);
    io->set_sda(io->ctx, 0);
    bus_delay(io);
    io->set_scl(io->ctx, 0);
}

static void bus_stop(const poteto_bus_io *io) {
    io->set_sda(io->ctx, 0);
    bus_delay(io);
    io->set_scl(io->ctx, 1);
    bus_delay(io);
    io->set_sda(io->ctx, 1);
    bus_delay(io);
}

static poteto_bus_status write_byte(const poteto_bus_io *io, uint8_t value) {
    int bit;
    int acked;
    for (bit = 7; bit >= 0; bit--) {
        int one = (value >> bit) & 1;
        io->set_sda(io->ctx, one);
        if (one && !io->read_sda(io->ctx)) {
            return POTETO_BUS_SDA_STUCK;
        }
        bus_delay(io);
        if (scl_release(io) != POTETO_BUS_ACK) {
            return POTETO_BUS_SCL_STUCK;
        }
        bus_delay(io);
        io->set_scl(io->ctx, 0);
    }
    io->set_sda(io->ctx, 1);
    bus_delay(io);
    if (scl_release(io) != POTETO_BUS_ACK) {
        return POTETO_BUS_SCL_STUCK;
    }
    acked = io->read_sda(io->ctx) == 0;
    bus_delay(io);
    io->set_scl(io->ctx, 0);
    return acked ? POTETO_BUS_ACK : POTETO_BUS_NACK;
}

static void bus_restart(const poteto_bus_io *io) {
    io->set_sda(io->ctx, 1);
    bus_delay(io);
    io->set_scl(io->ctx, 1);
    bus_delay(io);
    io->set_sda(io->ctx, 0);
    bus_delay(io);
    io->set_scl(io->ctx, 0);
}

static poteto_bus_status read_byte(const poteto_bus_io *io, uint8_t *value) {
    int bit;
    uint8_t got = 0;
    io->set_sda(io->ctx, 1);
    for (bit = 7; bit >= 0; bit--) {
        bus_delay(io);
        if (scl_release(io) != POTETO_BUS_ACK) {
            return POTETO_BUS_SCL_STUCK;
        }
        if (io->read_sda(io->ctx)) {
            got = (uint8_t)(got | (1U << bit));
        }
        bus_delay(io);
        io->set_scl(io->ctx, 0);
    }
    io->set_sda(io->ctx, 1);
    bus_delay(io);
    if (scl_release(io) != POTETO_BUS_ACK) {
        return POTETO_BUS_SCL_STUCK;
    }
    bus_delay(io);
    io->set_scl(io->ctx, 0);
    *value = got;
    return POTETO_BUS_ACK;
}

poteto_bus_status poteto_bus_read_reg(const poteto_bus_io *io, uint8_t addr_w, uint8_t reg, uint8_t addr_r,
                                      uint8_t *value) {
    poteto_bus_status status;
    if (io == NULL || value == NULL) {
        return POTETO_BUS_NACK;
    }
    status = bus_idle(io);
    if (status != POTETO_BUS_ACK) {
        return status;
    }
    bus_start(io);
    status = write_byte(io, addr_w);
    if (status == POTETO_BUS_ACK) {
        status = write_byte(io, reg);
    }
    if (status == POTETO_BUS_ACK) {
        bus_restart(io);
        status = write_byte(io, addr_r);
    }
    if (status == POTETO_BUS_ACK) {
        status = read_byte(io, value);
    }
    bus_stop(io);
    return status;
}

poteto_bus_status poteto_bus_txn(const poteto_bus_io *io, const uint8_t *bytes, int count) {
    poteto_bus_status status;
    int i;
    if (io == NULL || io->set_sda == NULL || io->set_scl == NULL || io->read_sda == NULL ||
        io->read_scl == NULL || bytes == NULL || count <= 0) {
        return POTETO_BUS_NACK;
    }
    status = bus_idle(io);
    if (status != POTETO_BUS_ACK) {
        return status;
    }
    bus_start(io);
    for (i = 0; i < count; i++) {
        status = write_byte(io, bytes[i]);
        if (status != POTETO_BUS_ACK) {
            bus_stop(io);
            return status;
        }
    }
    bus_stop(io);
    return POTETO_BUS_ACK;
}

const char *poteto_key3_caption(int opened) {
    return opened ? "\xEB\xA7\x88\xEC\x9D\xB4\xED\x81\xAC" : "\xEB\xA7\x88\xEC\x9D\xB4\xED\x81\xAC \xEC\x97\x86\xEC\x9D\x8C";
}
