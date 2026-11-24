idf_component_register(
    SRCS "app_main.c" "schedule.c" "frame_table.c" "rle.c" "blob.c" "say.c" "bubble.c" "bubble_slot.c" "pbf1.c" "key3.c" "key3_edge.c" "key3_bus.c" "key3_i2s.c" "companion.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_timer esp32_s3_touch_amoled_2_16 heap esp_partition esp_driver_gpio esp_driver_i2s esp_rom freertos espressif__esp_codec_dev
    EMBED_FILES "frames.bin"
)
