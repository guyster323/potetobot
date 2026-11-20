#include "bubble.h"

#include "say.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/* Board pins from the Waveshare ESP32-S3-Touch-AMOLED-2.16 BSP.
   Touch is polled with a short open-drain I2C loop so the factory
   image does not have to link the ESP-IDF I2C and CST9217 drivers. */
#define TOUCH_SCL GPIO_NUM_14
#define TOUCH_SDA GPIO_NUM_15
#define TOUCH_RST GPIO_NUM_40
#define TOUCH_INT GPIO_NUM_11
#define TOUCH_ADDR 0x5A
#define TOUCH_REG 0xD000
#define TOUCH_ACK 0xAB

#define SAY_SLOTS 8
#define LINE_H 18
#define ROUND_R 14
#define TEXT_X (POTETO_BUBBLE_X + 16)
#define TEXT_Y (POTETO_BUBBLE_Y + 16)
#define TEXT_W (POTETO_BUBBLE_W - 32)
#define TEXT_H (POTETO_BUBBLE_H - 32)
#define MAX_LINES 192

static const char *TAG = "bubble";

typedef struct {
    char phase[POTETO_SAY_PHASE_MAX];
    char text[POTETO_SAY_TEXT_MAX];
} say_slot_t;

typedef struct {
    const char *bytes;
    int length;
    int thinking;
} text_line_t;

static say_slot_t s_slots[SAY_SLOTS];
static int s_count;
static int s_origin;
static int s_scroll;
static int s_snap;
static SemaphoreHandle_t s_lock;
static int s_touch_ready;
static int s_pressed;
static int s_last_y;
static int s_drag;
static const uint8_t *s_font;
static size_t s_font_len;
static uint16_t s_ascii_count;
static uint16_t s_glyph_stride;
static uint32_t s_hangul_count;
static text_line_t s_lines[MAX_LINES];

static void i2c_delay(void) {
    esp_rom_delay_us(4);
}

static void line_mode(gpio_num_t pin) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(pin, 1);
}

static void scl_high(void) {
    int n;
    gpio_set_level(TOUCH_SCL, 1);
    for (n = 0; n < 40; n++) {
        if (gpio_get_level(TOUCH_SCL)) {
            break;
        }
        i2c_delay();
    }
}

static void i2c_start(void) {
    gpio_set_level(TOUCH_SDA, 1);
    scl_high();
    i2c_delay();
    gpio_set_level(TOUCH_SDA, 0);
    i2c_delay();
    gpio_set_level(TOUCH_SCL, 0);
}

static void i2c_stop(void) {
    gpio_set_level(TOUCH_SDA, 0);
    i2c_delay();
    scl_high();
    i2c_delay();
    gpio_set_level(TOUCH_SDA, 1);
    i2c_delay();
}

static int i2c_write_byte(uint8_t value) {
    int bit;
    int ack;
    for (bit = 7; bit >= 0; bit--) {
        gpio_set_level(TOUCH_SDA, (value >> bit) & 1);
        i2c_delay();
        scl_high();
        i2c_delay();
        gpio_set_level(TOUCH_SCL, 0);
    }
    gpio_set_level(TOUCH_SDA, 1);
    i2c_delay();
    scl_high();
    ack = gpio_get_level(TOUCH_SDA);
    i2c_delay();
    gpio_set_level(TOUCH_SCL, 0);
    return ack == 0;
}

static uint8_t i2c_read_byte(int ack) {
    int bit;
    uint8_t value = 0;
    gpio_set_level(TOUCH_SDA, 1);
    for (bit = 7; bit >= 0; bit--) {
        i2c_delay();
        scl_high();
        if (gpio_get_level(TOUCH_SDA)) {
            value = (uint8_t)(value | (1U << bit));
        }
        i2c_delay();
        gpio_set_level(TOUCH_SCL, 0);
    }
    gpio_set_level(TOUCH_SDA, ack ? 0 : 1);
    i2c_delay();
    scl_high();
    i2c_delay();
    gpio_set_level(TOUCH_SCL, 0);
    gpio_set_level(TOUCH_SDA, 1);
    return value;
}

static int touch_read(uint8_t *data, int length) {
    int i;
    i2c_start();
    if (!i2c_write_byte((uint8_t)(TOUCH_ADDR << 1)) || !i2c_write_byte(TOUCH_REG >> 8) ||
        !i2c_write_byte(TOUCH_REG & 0xFF)) {
        i2c_stop();
        return -1;
    }
    i2c_stop();
    esp_rom_delay_us(1500);
    i2c_start();
    if (!i2c_write_byte((uint8_t)((TOUCH_ADDR << 1) | 1))) {
        i2c_stop();
        return -1;
    }
    for (i = 0; i < length; i++) {
        data[i] = i2c_read_byte(i + 1 < length);
    }
    i2c_stop();
    return 0;
}

static void touch_init(void) {
    gpio_config_t rst = {
        .pin_bit_mask = 1ULL << TOUCH_RST,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config_t intr = {
        .pin_bit_mask = 1ULL << TOUCH_INT,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rst);
    gpio_config(&intr);
    gpio_set_level(TOUCH_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TOUCH_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    line_mode(TOUCH_SCL);
    line_mode(TOUCH_SDA);
    s_touch_ready = 1;
}

static int touch_point(int *x_out, int *y_out) {
    uint8_t data[10];
    uint16_t x;
    uint16_t y;
    uint16_t swap;
    if (!s_touch_ready || touch_read(data, 10) != 0) {
        return 0;
    }
    if (data[6] != TOUCH_ACK || (data[5] & 0x7F) == 0 || (data[0] & 0x0F) != 0x06) {
        return 0;
    }
    x = (uint16_t)((data[1] << 4) | (data[3] >> 4));
    y = (uint16_t)((data[2] << 4) | (data[3] & 0x0F));
    /* Same adjustment as bsp_display_start: mirror Y, then swap X and Y. */
    y = (uint16_t)(480 - y);
    swap = x;
    x = y;
    y = swap;
    if (x >= 480) {
        x = 479;
    }
    if (y >= 480) {
        y = 479;
    }
    *x_out = (int)x;
    *y_out = (int)y;
    return 1;
}

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void font_init(void) {
    const esp_partition_t *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x40, "font");
    const void *mapped = NULL;
    esp_partition_mmap_handle_t handle = 0;
    uint16_t gw;
    uint16_t gh;
    size_t glyphs;
    if (part == NULL) {
        ESP_LOGW(TAG, "font partition missing");
        return;
    }
    if (esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, &mapped, &handle) != ESP_OK) {
        ESP_LOGW(TAG, "font mmap failed");
        return;
    }
    s_font = (const uint8_t *)mapped;
    s_font_len = part->size;
    if (s_font_len < 16 || s_font[0] != 'P' || s_font[1] != 'B' || s_font[2] != 'F' || s_font[3] != '1') {
        ESP_LOGW(TAG, "font magic missing");
        s_font = NULL;
        return;
    }
    gw = read_u16(s_font + 4);
    gh = read_u16(s_font + 6);
    s_ascii_count = read_u16(s_font + 8);
    s_glyph_stride = read_u16(s_font + 10);
    s_hangul_count = read_u32(s_font + 12);
    glyphs = (size_t)s_ascii_count + (size_t)s_hangul_count;
    if (gw != 16 || gh != 16 || s_glyph_stride != 32 || 16u + glyphs * s_glyph_stride > s_font_len) {
        ESP_LOGW(TAG, "font atlas does not fit");
        s_font = NULL;
        s_ascii_count = 0;
        s_hangul_count = 0;
    }
}

static const uint8_t *font_find(uint32_t cp, int *width) {
    uint32_t index;
    size_t offset;
    if (s_font == NULL || s_glyph_stride == 0) {
        return NULL;
    }
    if (cp >= 32 && cp < 32u + s_ascii_count) {
        index = cp - 32u;
    } else if (cp >= 0xAC00 && cp < 0xAC00u + s_hangul_count) {
        index = s_ascii_count + (cp - 0xAC00u);
    } else {
        return NULL;
    }
    offset = 16u + (size_t)index * s_glyph_stride;
    if (offset + s_glyph_stride > s_font_len) {
        return NULL;
    }
    *width = 16;
    return s_font + offset;
}

static uint32_t utf8_next(const char **cursor, const char *end) {
    const uint8_t *s = (const uint8_t *)*cursor;
    const uint8_t *limit = (const uint8_t *)end;
    uint32_t cp;
    int need;
    if (s >= limit || *s == 0) {
        return 0;
    }
    if (*s < 0x80) {
        *cursor += 1;
        return *s;
    }
    if ((*s & 0xE0) == 0xC0) {
        cp = *s & 0x1F;
        need = 1;
    } else if ((*s & 0xF0) == 0xE0) {
        cp = *s & 0x0F;
        need = 2;
    } else if ((*s & 0xF8) == 0xF0) {
        cp = *s & 0x07;
        need = 3;
    } else {
        *cursor += 1;
        return 0xFFFD;
    }
    s++;
    if (s + need > limit) {
        *cursor = end;
        return 0xFFFD;
    }
    while (need > 0) {
        if ((*s & 0xC0) != 0x80) {
            *cursor = (const char *)s;
            return 0xFFFD;
        }
        cp = (cp << 6) | (*s & 0x3F);
        s++;
        need--;
    }
    *cursor = (const char *)s;
    return cp;
}

static int glyph_span(uint32_t cp) {
    int width = 0;
    if (cp == 0) {
        return 0;
    }
    if (font_find(cp, &width) != NULL) {
        return width;
    }
    if (cp >= 32 && cp <= 126) {
        return 8;
    }
    return 10;
}

static int inside_round(int x, int y, int left, int top, int width, int height, int radius) {
    int dx = 0;
    int dy = 0;
    int right = left + width - 1;
    int bottom = top + height - 1;
    if (x < left || y < top || x > right || y > bottom) {
        return 0;
    }
    if (x < left + radius) {
        dx = left + radius - x;
    } else if (x > right - radius) {
        dx = x - (right - radius);
    }
    if (y < top + radius) {
        dy = top + radius - y;
    } else if (y > bottom - radius) {
        dy = y - (bottom - radius);
    }
    return dx * dx + dy * dy <= radius * radius;
}

static uint16_t dim_pixel(uint16_t pixel) {
    pixel = (uint16_t)((pixel & 0xF7DE) >> 1);
    return (uint16_t)((pixel & 0xF7DE) >> 1);
}

static void plot(uint16_t *frame, int x, int y, uint16_t color) {
    if (x < 0 || y < 0 || x >= 480 || y >= 480) {
        return;
    }
    frame[(size_t)y * 480 + (size_t)x] = color;
}

static void paint_shape(uint16_t *frame) {
    int y;
    int x;
    for (y = POTETO_BUBBLE_Y; y < POTETO_BUBBLE_Y + POTETO_BUBBLE_H; y++) {
        for (x = POTETO_BUBBLE_X; x < POTETO_BUBBLE_X + POTETO_BUBBLE_W; x++) {
            int outer = inside_round(x, y, POTETO_BUBBLE_X, POTETO_BUBBLE_Y, POTETO_BUBBLE_W, POTETO_BUBBLE_H, ROUND_R);
            int inner = inside_round(x, y, POTETO_BUBBLE_X + 2, POTETO_BUBBLE_Y + 2, POTETO_BUBBLE_W - 4, POTETO_BUBBLE_H - 4, ROUND_R - 2);
            if (!outer) {
                continue;
            }
            if (inner) {
                frame[(size_t)y * 480 + (size_t)x] = dim_pixel(frame[(size_t)y * 480 + (size_t)x]);
            } else {
                frame[(size_t)y * 480 + (size_t)x] = 0xFFFF;
            }
        }
    }
}

static void blit_bits(uint16_t *frame, int x, int y, const uint8_t *bits, int width, int height, int stride, uint16_t color) {
    int row;
    int col;
    for (row = 0; row < height; row++) {
        int py = y + row;
        if (py < TEXT_Y || py >= TEXT_Y + TEXT_H) {
            continue;
        }
        for (col = 0; col < width; col++) {
            int px = x + col;
            if (px < TEXT_X || px >= TEXT_X + TEXT_W) {
                continue;
            }
            if (bits[row * stride + (col >> 3)] & (uint8_t)(0x80 >> (col & 7))) {
                plot(frame, px, py, color);
            }
        }
    }
}

static void draw_cp(uint16_t *frame, int x, int y, uint32_t cp, uint16_t color) {
    int width = 0;
    const uint8_t *bits = font_find(cp, &width);
    if (bits != NULL) {
        blit_bits(frame, x, y, bits, width, 16, width / 8, color);
        return;
    }
    plot(frame, x, y + 2, color);
    plot(frame, x + 8, y + 2, color);
    plot(frame, x, y + 12, color);
    plot(frame, x + 8, y + 12, color);
}

static int layout_lines(void) {
    int count = 0;
    int slot;
    for (slot = 0; slot < s_count; slot++) {
        const say_slot_t *item = &s_slots[(s_origin + slot) % SAY_SLOTS];
        const char *text = item->text;
        const char *end = text + strlen(text);
        const char *cursor = text;
        int thinking = strcmp(item->phase, "thinking") == 0;
        if (text[0] == '\0') {
            text = thinking ? "..." : " ";
            end = text + strlen(text);
            cursor = text;
        }
        while (cursor < end && count < MAX_LINES) {
            const char *line = cursor;
            const char *break_at = cursor;
            int width = 0;
            int broke = 0;
            while (cursor < end) {
                const char *next = cursor;
                uint32_t cp = utf8_next(&next, end);
                int span;
                if (cp == '\n') {
                    cursor = next;
                    broke = 1;
                    break;
                }
                span = glyph_span(cp);
                if (width + span > TEXT_W && line != cursor) {
                    if (break_at != line) {
                        cursor = break_at;
                    }
                    broke = 1;
                    break;
                }
                width += span;
                cursor = next;
                if (cp == ' ') {
                    break_at = cursor;
                }
            }
            s_lines[count].bytes = line;
            s_lines[count].length = (int)(cursor - line);
            s_lines[count].thinking = thinking;
            count++;
            if (!broke && cursor >= end) {
                break;
            }
            while (*cursor == ' ') {
                cursor++;
            }
        }
    }
    return count;
}

static void paint_text(uint16_t *frame, int line_count, int scroll) {
    int i;
    for (i = 0; i < line_count; i++) {
        int y = TEXT_Y + i * LINE_H - scroll;
        const char *cursor = s_lines[i].bytes;
        const char *end = cursor + s_lines[i].length;
        int x = TEXT_X;
        uint16_t color = s_lines[i].thinking ? 0xCE79 : 0xFFFF;
        if (y >= TEXT_Y + TEXT_H || y + 16 <= TEXT_Y) {
            continue;
        }
        while (cursor < end) {
            const char *next = cursor;
            uint32_t cp = utf8_next(&next, end);
            int span = glyph_span(cp);
            if (cp != ' ' && cp != '\n') {
                draw_cp(frame, x, y, cp, color);
            }
            x += span;
            cursor = next;
        }
    }
}

void poteto_bubble_init(void) {
    s_lock = xSemaphoreCreateMutex();
    font_init();
    touch_init();
    ESP_LOGI(TAG, "bubble %d,%d %dx%d font=%s", POTETO_BUBBLE_X, POTETO_BUBBLE_Y, POTETO_BUBBLE_W, POTETO_BUBBLE_H,
             s_font != NULL ? "partition" : "missing");
}

void poteto_bubble_push(const char *phase, const char *text) {
    say_slot_t *slot;
    if (phase == NULL || text == NULL || s_lock == NULL) {
        return;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }
    if (strcmp(phase, "answer") == 0 && s_count > 0) {
        say_slot_t *last = &s_slots[(s_origin + s_count - 1) % SAY_SLOTS];
        if (strcmp(last->phase, "thinking") == 0) {
            snprintf(last->phase, sizeof(last->phase), "%s", phase);
            snprintf(last->text, sizeof(last->text), "%s", text);
            s_snap = 1;
            xSemaphoreGive(s_lock);
            return;
        }
    }
    if (s_count < SAY_SLOTS) {
        slot = &s_slots[s_count];
        s_count++;
    } else {
        slot = &s_slots[s_origin];
        s_origin = (s_origin + 1) % SAY_SLOTS;
    }
    snprintf(slot->phase, sizeof(slot->phase), "%s", phase);
    snprintf(slot->text, sizeof(slot->text), "%s", text);
    s_snap = 1;
    xSemaphoreGive(s_lock);
}

void poteto_bubble_poll(void) {
    int x = 0;
    int y = 0;
    int down;
    if (s_lock == NULL) {
        return;
    }
    down = touch_point(&x, &y);
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    if (!down) {
        s_pressed = 0;
        s_drag = 0;
    } else if (!s_pressed) {
        s_pressed = 1;
        s_last_y = y;
        s_drag = 1;
    } else if (s_drag) {
        s_scroll += s_last_y - y;
        s_last_y = y;
        s_snap = 0;
        if (s_scroll < 0) {
            s_scroll = 0;
        }
    }
    xSemaphoreGive(s_lock);
}

void poteto_bubble_paint(uint16_t *frame) {
    int line_count;
    int content;
    int max_scroll;
    int scroll;
    if (frame == NULL || s_lock == NULL || s_count <= 0) {
        return;
    }
    if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }
    line_count = layout_lines();
    content = line_count * LINE_H;
    max_scroll = content > TEXT_H ? content - TEXT_H : 0;
    if (s_snap) {
        s_scroll = max_scroll;
        s_snap = 0;
    }
    if (s_scroll > max_scroll) {
        s_scroll = max_scroll;
    }
    if (s_scroll < 0) {
        s_scroll = 0;
    }
    scroll = s_scroll;
    paint_shape(frame);
    paint_text(frame, line_count, scroll);
    xSemaphoreGive(s_lock);
}
