idf_component_register(
    SRCS "app_main.c" "schedule.c" "frame_table.c" "rle.c" "blob.c" "say.c" "bubble.c"
    INCLUDE_DIRS "."
    REQUIRES esp_lcd esp_timer esp32_s3_touch_amoled_2_16 heap esp_partition esp_driver_gpio esp_rom
    EMBED_FILES "frames.bin"
)
