#include "bubble_slot.h"
#include "key3_bus.h"
#include "key3_edge.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    return 1;
}

static int expect_slot(int index, const char *phase, const char *text, const char *message) {
    const char *got_phase = poteto_slots_phase(index);
    const char *got_text = poteto_slots_text(index);
    if (got_phase == NULL || got_text == NULL || strcmp(got_phase, phase) != 0 || strcmp(got_text, text) != 0) {
        fprintf(stderr, "%s (phase=%s text=%s)\n", message, got_phase == NULL ? "?" : got_phase,
                got_text == NULL ? "?" : got_text);
        return 1;
    }
    return 0;
}

static int test_slots(void) {
    poteto_slots_reset();
    if (poteto_slots_push_txt("thinking", "...") != 0 || poteto_slots_push_mic("\xEB\xA7\x88\xEC\x9D\xB4\xED\x81\xAC") != 0 ||
        poteto_slots_push_txt("answer", "\xEA\xB3\xA0\xEA\xB5\xAC\xEB\xA7\x88") != 0) {
        return fail("slot push failed");
    }
    if (poteto_slots_count() != 3) {
        return fail("thinking disappeared around a mic note");
    }
    if (expect_slot(0, "thinking", "...", "thinking slot changed") ||
        expect_slot(1, "mic", "\xEB\xA7\x88\xEC\x9D\xB4\xED\x81\xAC", "mic slot") ||
        expect_slot(2, "answer", "\xEA\xB3\xA0\xEA\xB5\xAC\xEB\xA7\x88", "answer slot")) {
        return 1;
    }
    poteto_slots_reset();
    if (poteto_slots_push_txt("thinking", "...") != 0 || poteto_slots_push_txt("answer", "done") != 0) {
        return fail("txt replace push failed");
    }
    if (poteto_slots_count() != 1 || expect_slot(0, "answer", "done", "adjacent answer should replace thinking")) {
        return 1;
    }
    return 0;
}

typedef struct {
    int sda_out;
    int scl_out;
    int sda_stuck;
    int scl_stuck;
    int bits;
    int ack;
} fake_bus;

static void set_sda(void *ctx, int high) {
    ((fake_bus *)ctx)->sda_out = high ? 1 : 0;
}

static void set_scl(void *ctx, int high) {
    fake_bus *bus = (fake_bus *)ctx;
    int rising = high && !bus->scl_out;
    bus->scl_out = high ? 1 : 0;
    if (rising && !bus->scl_stuck) {
        bus->bits++;
    }
}

static int read_sda(void *ctx) {
    fake_bus *bus = (fake_bus *)ctx;
    if (bus->sda_stuck) {
        return 0;
    }
    if (bus->ack && bus->scl_out && bus->bits > 0 && (bus->bits % 9) == 0) {
        return 0;
    }
    return bus->sda_out;
}

static int read_scl(void *ctx) {
    fake_bus *bus = (fake_bus *)ctx;
    if (bus->scl_stuck) {
        return 0;
    }
    return bus->scl_out;
}

static poteto_bus_status run_bus(int sda_stuck, int scl_stuck, int ack) {
    fake_bus bus = {
        .sda_out = 1,
        .scl_out = 1,
        .sda_stuck = sda_stuck,
        .scl_stuck = scl_stuck,
        .bits = 0,
        .ack = ack,
    };
    poteto_bus_io io = {
        .set_sda = set_sda,
        .set_scl = set_scl,
        .read_sda = read_sda,
        .read_scl = read_scl,
        .delay = NULL,
        .ctx = &bus,
    };
    uint8_t bytes[3] = {0x80, 0x00, 0x41};
    return poteto_bus_txn(&io, bytes, 3);
}

static int test_bus(void) {
    if (run_bus(0, 0, 1) != POTETO_BUS_ACK) {
        return fail("slave ack was not an open");
    }
    if (run_bus(0, 0, 0) != POTETO_BUS_NACK) {
        return fail("address nack was not distinguished");
    }
    if (run_bus(1, 0, 1) != POTETO_BUS_SDA_STUCK) {
        return fail("stuck sda looked like an ack");
    }
    if (run_bus(0, 1, 1) != POTETO_BUS_SCL_STUCK) {
        return fail("stuck scl looked like an ack");
    }
    if (strcmp(poteto_key3_caption(1), "\xEB\xA7\x88\xEC\x9D\xB4\xED\x81\xAC") != 0) {
        return fail("open caption");
    }
    if (strcmp(poteto_key3_caption(0), "\xEB\xA7\x88\xEC\x9D\xB4\xED\x81\xAC \xEC\x97\x86\xEC\x9D\x8C") != 0) {
        return fail("fault caption");
    }
    return 0;
}

static int test_edge(void) {
    poteto_key3_edge st;
    memset(&st, 0, sizeof(st));
    if (poteto_key3_edge_step(&st, 1, 1) != 0) {
        return fail("boot first sample opened");
    }
    memset(&st, 0, sizeof(st));
    if (poteto_key3_edge_step(&st, 0, 1) != 0 || poteto_key3_edge_step(&st, 0, 1) != 0) {
        return fail("boot held sample opened");
    }
    if (poteto_key3_edge_step(&st, 1, 0) != 0) {
        return fail("release after boot hold opened");
    }
    if (poteto_key3_edge_step(&st, 0, 1) != 1) {
        return fail("press after boot hold did not open");
    }
    if (poteto_key3_edge_step(&st, 0, 1) != 0 || poteto_key3_edge_step(&st, 0, 0) != 0) {
        return fail("hold repeated");
    }
    if (poteto_key3_edge_step(&st, 1, 0) != 0 || poteto_key3_edge_step(&st, 0, 1) != 1) {
        return fail("repress did not open once");
    }
    memset(&st, 0, sizeof(st));
    if (poteto_key3_edge_step(&st, 1, 0) != 0) {
        return fail("boot high sample opened");
    }
    if (poteto_key3_edge_step(&st, 1, 1) != 1) {
        return fail("short tap between samples was missed");
    }
    if (poteto_key3_edge_step(&st, 1, 0) != 0) {
        return fail("short tap repeated");
    }
    return 0;
}

int main(void) {
    if (test_slots() || test_bus() || test_edge()) {
        return 1;
    }
    printf("key3 ok\n");
    return 0;
}
