idf_component_register(SRCS "oled_driver.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_driver_i2c esp_lcd)
