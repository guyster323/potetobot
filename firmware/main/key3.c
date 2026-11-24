#include "key3.h"

#include "key3_bus.h"
#include "key3_edge.h"
#include "key3_i2s.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_attr.h"
#include "esp_codec_dev_types.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "es7210_adc.h"

/* Touch already bit-bangs this bus. The IDF I2C driver would take the
   same pins and break bubble scroll. I2S RX is separate and does not
   call bsp_i2c_init. */
#define KEY3_GPIO GPIO_NUM_18
#define BUS_SCL GPIO_NUM_14
#define BUS_SDA GPIO_NUM_15
#define ES7210_ADDR_W 0x80
#define ES7210_ADDR_R 0x81

static const char *TAG = "key3";
static int s_gpio_ready;
static volatile int s_fall;
static poteto_key3_edge s_edge;
static poteto_bus_status s_bus;
static const audio_codec_if_t *s_codec;
static const audio_codec_ctrl_if_t *s_ctrl_ptr;
static unsigned s_seq;
static size_t s_emitted;
static int s_active;
static int s_i2s_bad;
static const char *s_reason;
static poteto_key3_face s_face;
static portMUX_TYPE s_face_mux = portMUX_INITIALIZER_UNLOCKED;

static void gpio_sda(void *ctx, int high) {
    (void)ctx;
    gpio_set_level(BUS_SDA, high);
}

static void gpio_scl(void *ctx, int high) {
    (void)ctx;
    gpio_set_level(BUS_SCL, high);
}

static int gpio_read_sda(void *ctx) {
    (void)ctx;
    return gpio_get_level(BUS_SDA);
}

static int gpio_read_scl(void *ctx) {
    (void)ctx;
    return gpio_get_level(BUS_SCL);
}

static void gpio_delay(void *ctx) {
    (void)ctx;
    esp_rom_delay_us(4);
}

static const poteto_bus_io s_io = {
    .set_sda = gpio_sda,
    .set_scl = gpio_scl,
    .read_sda = gpio_read_sda,
    .read_scl = gpio_read_scl,
    .delay = gpio_delay,
    .ctx = NULL,
};

static void IRAM_ATTR key3_isr(void *arg) {
    (void)arg;
    s_fall = 1;
}

static int ctrl_open(const audio_codec_ctrl_if_t *ctrl, void *cfg, int cfg_size) {
    (void)ctrl;
    (void)cfg;
    (void)cfg_size;
    return ESP_CODEC_DEV_OK;
}

static bool ctrl_is_open(const audio_codec_ctrl_if_t *ctrl) {
    (void)ctrl;
    return true;
}

static int ctrl_read(const audio_codec_ctrl_if_t *ctrl, int reg, int reg_len, void *data, int data_len) {
    uint8_t value = 0;
    (void)ctrl;
    if (data == NULL || reg_len != 1 || data_len < 1) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    s_bus = poteto_bus_read_reg(&s_io, ES7210_ADDR_W, (uint8_t)reg, ES7210_ADDR_R, &value);
    if (s_bus != POTETO_BUS_ACK) {
        return ESP_CODEC_DEV_READ_FAIL;
    }
    ((uint8_t *)data)[0] = value;
    return ESP_CODEC_DEV_OK;
}

static int ctrl_write(const audio_codec_ctrl_if_t *ctrl, int reg, int reg_len, void *data, int data_len) {
    uint8_t bytes[3];
    (void)ctrl;
    if (data == NULL || reg_len != 1 || data_len < 1) {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    bytes[0] = ES7210_ADDR_W;
    bytes[1] = (uint8_t)reg;
    bytes[2] = ((const uint8_t *)data)[0];
    s_bus = poteto_bus_txn(&s_io, bytes, 3);
    if (s_bus != POTETO_BUS_ACK) {
        return ESP_CODEC_DEV_WRITE_FAIL;
    }
    return ESP_CODEC_DEV_OK;
}

static int ctrl_close(const audio_codec_ctrl_if_t *ctrl) {
    (void)ctrl;
    return ESP_CODEC_DEV_OK;
}

static const audio_codec_ctrl_if_t s_ctrl = {
    .open = ctrl_open,
    .is_open = ctrl_is_open,
    .read_reg = ctrl_read,
    .write_reg = ctrl_write,
    .close = ctrl_close,
};

static int open_es7210(void) {
    es7210_codec_cfg_t cfg = {
        .ctrl_if = s_ctrl_ptr,
        .master_mode = false,
        .mic_selected = ES7210_SEL_MIC1 | ES7210_SEL_MIC2,
    };
    int err;
    s_bus = POTETO_BUS_ACK;
    if (s_codec == NULL) {
        s_codec = es7210_codec_new(&cfg);
        if (s_codec == NULL) {
            return -1;
        }
    } else if (s_codec->open == NULL || s_codec->open(s_codec, &cfg, (int)sizeof(cfg)) != ESP_CODEC_DEV_OK) {
        return -1;
    }
    if (s_codec->enable == NULL) {
        return -1;
    }
    if (s_codec->enable(s_codec, false) != ESP_CODEC_DEV_OK) {
        return -1;
    }
    err = s_codec->enable(s_codec, true);
    return err == ESP_CODEC_DEV_OK ? 0 : -1;
}

static const char *bus_name(poteto_bus_status status) {
    if (status == POTETO_BUS_NACK) {
        return "nack";
    }
    if (status == POTETO_BUS_SDA_STUCK) {
        return "sda";
    }
    if (status == POTETO_BUS_SCL_STUCK) {
        return "scl";
    }
    return "absent";
}

void poteto_key3_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << KEY3_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    esp_err_t err;
    s_ctrl_ptr = &s_ctrl;
    s_edge.seen = 0;
    s_edge.level = 1;
    s_edge.armed = 0;
    s_fall = 0;
    gpio_config(&cfg);
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "key3 isr service %d", (int)err);
    }
    err = gpio_isr_handler_add(KEY3_GPIO, key3_isr, NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "key3 isr add %d", (int)err);
    }
    s_gpio_ready = 1;
    ESP_LOGI(TAG, "key3 gpio18 armed");
}

static int emit_line(poteto_key3_line_fn fn, void *ctx, const char *line) {
    if (fn == NULL || line == NULL || fn(line, ctx) != 0) {
        s_reason = "queue";
        return -1;
    }
    return 0;
}

static void emit_ready(poteto_companion *board, poteto_key3_line_fn fn, void *ctx, unsigned utt) {
    char line[1600];
    while (s_reason == NULL && !board->overflow && board->voice != NULL &&
           board->voice_len - s_emitted >= POTETO_AUD_CHUNK) {
        if (poteto_aud_chunk(utt, s_seq, board->voice + s_emitted, POTETO_AUD_CHUNK, line, sizeof(line)) != 0 ||
            emit_line(fn, ctx, line) != 0) {
            board->overflow = 1;
            if (s_reason == NULL) {
                s_reason = "overflow";
            }
            return;
        }
        s_emitted += POTETO_AUD_CHUNK;
        s_seq++;
    }
}

static void finish_utterance(poteto_companion *board, poteto_key3_line_fn fn, void *ctx, unsigned utt) {
    char line[1600];
    size_t rem;
    const char *reason = s_reason;
    if (!s_active) {
        return;
    }
    if (reason == NULL && (board->overflow || s_i2s_bad || board->voice == NULL)) {
        reason = board->overflow ? "overflow" : "i2s";
    }
    if (reason == NULL && board->voice_len == 0) {
        reason = "empty";
    }
    if (reason == NULL) {
        rem = board->voice_len - s_emitted;
        if ((rem % 2) != 0) {
            reason = "odd";
        } else if (rem > 0 &&
                   (poteto_aud_chunk(utt, s_seq, board->voice + s_emitted, rem, line, sizeof(line)) != 0 ||
                    emit_line(fn, ctx, line) != 0)) {
            reason = s_reason != NULL ? s_reason : "overflow";
        } else {
            if (rem > 0) {
                s_seq++;
            }
            if (poteto_aud_end(utt, board->voice_len, s_seq, line, sizeof(line)) != 0 || emit_line(fn, ctx, line) != 0) {
                reason = "queue";
            }
        }
    }
    if (reason != NULL) {
        if (poteto_aud_drop(utt, reason, line, sizeof(line)) == 0) {
            (void)emit_line(fn, ctx, line);
        }
    }
    s_active = 0;
    s_reason = NULL;
    s_emitted = 0;
    s_seq = 0;
}

static void note_face(const poteto_companion *board) {
    poteto_key3_face next;
    next.changed = 1;
    next.uplink = NULL;
    next.uplink_len = 0;
    snprintf(next.expression, sizeof(next.expression), "%s", board->expression);
    taskENTER_CRITICAL(&s_face_mux);
    s_face = next;
    taskEXIT_CRITICAL(&s_face_mux);
}

void poteto_key3_service(poteto_key3_line_fn fn, void *ctx) {
    int level;
    int fell;
    uint8_t chunk[2048];
    size_t chunk_len = 0;
    size_t ignored = 0;
    int changed;
    poteto_companion *board;
    if (!s_gpio_ready) {
        vTaskDelay(pdMS_TO_TICKS(20));
        return;
    }
    gpio_intr_disable(KEY3_GPIO);
    fell = s_fall;
    s_fall = 0;
    gpio_intr_enable(KEY3_GPIO);
    level = gpio_get_level(KEY3_GPIO);
    if (poteto_key3_edge_step(&s_edge, level, fell)) {
        int i2s_ok = poteto_i2s_open() == 0;
        int opened = open_es7210() == 0;
        s_i2s_bad = !(i2s_ok && opened);
        if (i2s_ok && opened) {
            ESP_LOGI(TAG, "es7210 open");
        } else if (!opened) {
            ESP_LOGI(TAG, "es7210 %s", bus_name(s_bus));
        } else {
            ESP_LOGI(TAG, "i2s closed");
        }
    }
    board = poteto_companion_board();
    if (level == 0 && !s_i2s_bad && board->overflow == 0) {
        poteto_i2s_read(chunk, sizeof(chunk), &chunk_len);
    } else if (level == 0 && (s_i2s_bad || board->overflow)) {
        size_t discarded = 0;
        poteto_i2s_read(chunk, sizeof(chunk), &discarded);
        chunk_len = 0;
    }
    {
        unsigned previous = board->voice_utt;
        changed = poteto_companion_button(board, level, chunk_len == 0 ? NULL : chunk, chunk_len, NULL, 0, &ignored);
        if (changed && strcmp(board->expression, "listening") == 0) {
            if (s_active) {
                s_reason = "repress";
                finish_utterance(board, fn, ctx, previous);
            }
            s_seq = 0;
            s_emitted = 0;
            s_reason = NULL;
            s_active = 1;
            note_face(board);
        } else if (changed) {
            note_face(board);
        }
    }
    if (level == 0 && s_active) {
        emit_ready(board, fn, ctx, board->voice_utt);
    }
    if (changed && strcmp(board->expression, "thinking") == 0) {
        finish_utterance(board, fn, ctx, board->voice_utt);
    }
    if (level != 0) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void poteto_key3_take_face(poteto_key3_face *face) {
    if (face == NULL) {
        return;
    }
    taskENTER_CRITICAL(&s_face_mux);
    *face = s_face;
    s_face.changed = 0;
    taskEXIT_CRITICAL(&s_face_mux);
}
