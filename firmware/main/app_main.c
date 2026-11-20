#include "blob.h"
#include "bubble.h"
#include "rle.h"
#include "say.h"
#include "schedule.h"

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "bsp/display.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

extern const uint8_t frames_bin_start[] asm("_binary_frames_bin_start");
extern const uint8_t frames_bin_end[] asm("_binary_frames_bin_end");

static const char *TAG = "potetobot";
/* One QSPI burst stays inside internal DMA memory. 480*24*2 = 23040 bytes. */
static const int k_strip_rows = 24;
static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_frame;
static char s_character[16] = "poteto";
static char s_expression[16] = "idle";
static int64_t s_started_us;
static SemaphoreHandle_t s_draw_lock;

static void draw_current(void) {
    poteto_frame_t frame;
    const uint8_t *payload = NULL;
    size_t payload_len = 0;
    int elapsed_ms;
    size_t decoded;
    size_t i;

    if (s_draw_lock == NULL || xSemaphoreTake(s_draw_lock, pdMS_TO_TICKS(200)) != pdTRUE) {
        return;
    }
    elapsed_ms = (int)((esp_timer_get_time() - s_started_us) / 1000);
    if (poteto_frame_at(s_character, s_expression, elapsed_ms, &frame) != 0) {
        xSemaphoreGive(s_draw_lock);
        return;
    }
    if (frame.frame_count <= 0 ||
        poteto_blob_find(frames_bin_start, (size_t)(frames_bin_end - frames_bin_start), frame.character,
                         frame.expression, frame.frame_index, &payload, &payload_len) != 0) {
        memset(s_frame, 0, (size_t)POTETO_CANVAS * POTETO_CANVAS * sizeof(uint16_t));
    } else {
        decoded = poteto_rle_decode(payload, payload_len, s_frame, (size_t)POTETO_CANVAS * POTETO_CANVAS);
        if (decoded != (size_t)POTETO_CANVAS * POTETO_CANVAS) {
            memset(s_frame, 0, (size_t)POTETO_CANVAS * POTETO_CANVAS * sizeof(uint16_t));
        }
    }
    poteto_bubble_paint(s_frame);
    for (i = 0; i < (size_t)POTETO_CANVAS * POTETO_CANVAS; i++) {
        uint16_t value = s_frame[i];
        s_frame[i] = (uint16_t)((value >> 8) | (value << 8));
    }
    for (i = 0; i < (size_t)POTETO_CANVAS; i += (size_t)k_strip_rows) {
        int y = (int)i;
        int rows = k_strip_rows;
        if (y + rows > POTETO_CANVAS) {
            rows = POTETO_CANVAS - y;
        }
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, POTETO_CANVAS, y + rows, s_frame + (size_t)y * POTETO_CANVAS);
    }
    xSemaphoreGive(s_draw_lock);
}

static void apply_selection(const char *line) {
    poteto_frame_t frame;
    char ack[128];
    int ack_len;

    if (poteto_apply_line(line, &frame) != 0) {
        return;
    }
    snprintf(s_character, sizeof(s_character), "%s", frame.character);
    snprintf(s_expression, sizeof(s_expression), "%s", frame.expression);
    s_started_us = esp_timer_get_time();
    ack_len = snprintf(ack, sizeof(ack), "ACK %s %s\n", frame.character, frame.expression);
    if (ack_len > 0) {
        fwrite(ack, 1, (size_t)ack_len, stdout);
        fflush(stdout);
    }
    ESP_LOGI(TAG, "loop %s %s", frame.character, frame.expression);
    draw_current();
}

static void serial_task(void *arg) {
    static char line[640];
    (void)arg;
    while (1) {
        char phase[POTETO_SAY_PHASE_MAX];
        char text[POTETO_SAY_TEXT_MAX];
        if (fgets(line, sizeof(line), stdin) == NULL) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (poteto_apply_say(line, phase, sizeof(phase), text, sizeof(text)) == 0) {
            char ack[48];
            int ack_len;
            poteto_bubble_push(phase, text);
            ack_len = snprintf(ack, sizeof(ack), "ACK say %s\n", phase);
            if (ack_len > 0) {
                fwrite(ack, 1, (size_t)ack_len, stdout);
                fflush(stdout);
            }
            ESP_LOGI(TAG, "say %s", phase);
            draw_current();
            continue;
        }
        apply_selection(line);
    }
}

static void play_task(void *arg) {
    (void)arg;
    while (1) {
        poteto_bubble_poll();
        draw_current();
        vTaskDelay(pdMS_TO_TICKS(66));
    }
}

void app_main(void) {
    bsp_display_config_t config = {
        .max_transfer_sz = POTETO_CANVAS * k_strip_rows * (int)sizeof(uint16_t),
    };
    esp_lcd_panel_io_handle_t io = NULL;
    ESP_ERROR_CHECK(bsp_display_new(&config, &s_panel, &io));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
    bsp_display_brightness_set(80);

    s_draw_lock = xSemaphoreCreateMutex();
    s_frame = heap_caps_malloc((size_t)POTETO_CANVAS * POTETO_CANVAS * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_frame == NULL) {
        ESP_LOGE(TAG, "frame buffer alloc failed");
        return;
    }
    s_started_us = esp_timer_get_time();
    poteto_bubble_init();
    draw_current();
    xTaskCreate(play_task, "play", 8192, NULL, 4, NULL);
    xTaskCreate(serial_task, "serial", 8192, NULL, 5, NULL);
}
