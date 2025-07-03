/**
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025 franzininho
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file oled_driver.c
 *
 * @brief Implementacao do driver do display OLED.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 03 de Julho de 2025
 */

#include <string.h>
#include "oled_driver.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"

#define CONFIG_OLED_LCD_CMD_BITS           8
#define CONFIG_OLED_LCD_PARAM_BITS         8

#define COLLUMNS        (CONFIG_OLED_LCD_H_RES / 8)
#define ROWS            (CONFIG_OLED_LCD_V_RES)
#define DISPLAY_SIZE    (ROWS * COLLUMNS)


struct driver_state {

/* Status de inicializacao */
  bool initialised;

/* Handle para o painel */
  esp_lcd_panel_handle_t panel_handle;

/* Frame buffer do display */
  uint8_t frame_buffer[DISPLAY_SIZE];

/* Variavel para verificar possivel erro */
  esp_err_t rc;
};

static struct driver_state s_d_state = {0};
static const char *s_TAG = "OLED_D";

esp_err_t OledInit(void) {

  if(s_d_state.initialised)
    return ESP_ERR_NOT_ALLOWED;

  ESP_LOGI(s_TAG, "Initialize I2C bus");
  i2c_master_bus_handle_t i2c_bus = NULL;
  i2c_master_bus_config_t bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .i2c_port = CONFIG_OLED_I2C_BUS_PORT,
    .sda_io_num = CONFIG_OLED_PIN_NUM_SDA,
    .scl_io_num = CONFIG_OLED_PIN_NUM_SCL,
    .flags.enable_internal_pullup = true,
  };
  if ((s_d_state.rc = i2c_new_master_bus(&bus_config, &i2c_bus)))
    return s_d_state.rc;

  ESP_LOGI(s_TAG, "Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t io_config = {
    .dev_addr = CONFIG_OLED_I2C_HW_ADDR,
    .scl_speed_hz = CONFIG_OLED_PIXEL_CLOCK_HZ,
    .control_phase_bytes = 1,               // According to SSD1306 datasheet
    .lcd_cmd_bits = CONFIG_OLED_LCD_CMD_BITS,   // According to SSD1306 datasheet
    .lcd_param_bits = CONFIG_OLED_LCD_CMD_BITS, // According to SSD1306 datasheet
    .dc_bit_offset = 6,                     // According to SSD1306 datasheet
  };
  if ((s_d_state.rc = esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle)))
    return s_d_state.rc;

  ESP_LOGI(s_TAG, "Install SSD1306 panel driver");
  esp_lcd_panel_dev_config_t panel_config = {
    .bits_per_pixel = 1,
    .reset_gpio_num = CONFIG_OLED_PIN_NUM_RST,
  };
  esp_lcd_panel_ssd1306_config_t ssd1306_config = {
    .height = CONFIG_OLED_LCD_V_RES,
  };
  panel_config.vendor_config = &ssd1306_config;
  if ((s_d_state.rc = esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &s_d_state.panel_handle)))
    return s_d_state.rc;

  if ((s_d_state.rc = esp_lcd_panel_reset(s_d_state.panel_handle)))
    return s_d_state.rc;
  if ((s_d_state.rc = esp_lcd_panel_init(s_d_state.panel_handle)))
    return s_d_state.rc;
  if((s_d_state.rc = esp_lcd_panel_disp_on_off(s_d_state.panel_handle, true)))
    return s_d_state.rc;

  //Reduce contrast (default 0x7F);
  if ((s_d_state.rc = esp_lcd_panel_io_tx_param(io_handle, 0x81, (uint8_t[]){CONFIG_OLED_LCD_CONTRAST}, 1)))
    return s_d_state.rc;

  //Fix display origin in top-left
  if ((s_d_state.rc = esp_lcd_panel_mirror(s_d_state.panel_handle, true, true)))
    return s_d_state.rc;

  s_d_state.initialised = true;
  return s_d_state.rc;
}

esp_err_t OledDrawBitmap(int x_size, int y_size, int x_offset, int y_offset, const uint8_t* bitmap) {

  if (s_d_state.initialised) {
    uint8_t temp_buffer[DISPLAY_SIZE];
    memset(s_d_state.frame_buffer, 0, DISPLAY_SIZE);
    memset(temp_buffer,            0, DISPLAY_SIZE);

    x_size /= 8; x_offset /= 8;
    if((x_size + x_offset > COLLUMNS) || (y_size + y_offset > ROWS || !bitmap))
      return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < y_size; i++) {
      memcpy(temp_buffer + x_offset + (i * COLLUMNS) + (y_offset * COLLUMNS), bitmap + (i * x_size), x_size);
    }

    int index = 0;
    for (int i = 0; index < DISPLAY_SIZE; i++) {
      int modulo_byte = i % 8;
      int k = (i / 64) % 16 + (i / 1024) * 128;
      int pos = (7 - ((i / 8) % 8));
      uint8_t bit = (temp_buffer[(modulo_byte * COLLUMNS) + k] >> pos) & 0x01;
      s_d_state.frame_buffer[index] |= bit << modulo_byte;
      if (modulo_byte == 7)
        index++;
    }

    esp_lcd_panel_draw_bitmap(s_d_state.panel_handle, 0, 0, 128, 64, s_d_state.frame_buffer);
    return ESP_OK;
  }
  return ESP_ERR_INVALID_STATE;
}
