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
 * @file oled_exemplo.c
 *
 * @brief Aplicacao exemplo para desenhar no display OLED.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 03 de Julho de 2025
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "images.h"
#include "oled_driver.h"

static char *s_TAG = "app_main";

/**
 * @brief Loop principal
 *
 * Nao deve retornar.
 *
 */
void app_main(void) {

  ESP_LOGI(s_TAG, "Executando oled_exemplo.c\n"
           "Confirme que os jumpers de ativacao do display "
           "encontram-se instalados");

  /* Inicia o driver do display */
  if (OledInit() != ESP_OK) {
    ESP_LOGE(s_TAG, "Erro ao inicializar o driver do display...\n");
    for(;;){}
  }

  /* Loop infinito da aplicacao */
  while (true) {
    OledDrawBitmap(128, 64, 0, 0, franzininho_logo);
    vTaskDelay(pdMS_TO_TICKS(5000));
    for (int i = 0; i < 3 * FRAME_COUNT; i++) {
      OledDrawBitmap(FRAME_WIDTH, FRAME_HEIGHT, FRAME_START_X, FRAME_START_Y,
                     coffee[i % FRAME_COUNT]);
      vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY));
    }
    for (int i = 0; i < 3 * FRAME_COUNT; i++) {
      OledDrawBitmap(FRAME_WIDTH, FRAME_HEIGHT, FRAME_START_X, FRAME_START_Y,
                     skate[i % FRAME_COUNT]);
      vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY));
    }
  }

  /* Nao deve chegar aqui!! */
  return;
}
