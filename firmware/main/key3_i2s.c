#include "key3_i2s.h"

#include "driver/i2s_std.h"
#include "esp_log.h"

/* Pins match bsp/esp32_s3_touch_amoled_2_16.h. GPIO14 and GPIO15 stay with
   the bit-banged touch bus, so this file never calls bsp_i2c_init.
   I2S port 1 is CONFIG_BSP_I2S_NUM. Speaker DOUT is left unused. */
#define POTETO_I2S_PORT I2S_NUM_1
#define POTETO_I2S_SCLK GPIO_NUM_9
#define POTETO_I2S_MCLK GPIO_NUM_42
#define POTETO_I2S_LCLK GPIO_NUM_45
#define POTETO_I2S_DIN GPIO_NUM_10

static const char *TAG = "key3_i2s";
static i2s_chan_handle_t s_rx;
static int s_open;

int poteto_i2s_open(void) {
    i2s_chan_config_t chan = I2S_CHANNEL_DEFAULT_CONFIG(POTETO_I2S_PORT, I2S_ROLE_MASTER);
    i2s_std_config_t std = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = POTETO_I2S_MCLK,
            .bclk = POTETO_I2S_SCLK,
            .ws = POTETO_I2S_LCLK,
            .dout = I2S_GPIO_UNUSED,
            .din = POTETO_I2S_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    esp_err_t err;
    if (s_open) {
        return 0;
    }
    std.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    chan.auto_clear = true;
    err = i2s_new_channel(&chan, NULL, &s_rx);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2s channel %d", (int)err);
        return -1;
    }
    err = i2s_channel_init_std_mode(s_rx, &std);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2s mode %d", (int)err);
        return -1;
    }
    err = i2s_channel_enable(s_rx);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2s enable %d", (int)err);
        return -1;
    }
    s_open = 1;
    ESP_LOGI(TAG, "i2s rx 16000");
    return 0;
}

void poteto_i2s_read(uint8_t *dst, size_t cap, size_t *n) {
    size_t got = 0;
    if (n != NULL) {
        *n = 0;
    }
    if (!s_open || dst == NULL || cap == 0 || n == NULL) {
        return;
    }
    if (i2s_channel_read(s_rx, dst, cap, &got, 20) != ESP_OK) {
        return;
    }
    *n = got;
}
