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
 * @file buzzer_exemplo.c
 *
 * @brief Aplicacao exemplo para controlar o buzzer.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 29 de Maio de 2025
 */

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer_driver.h"

static char *s_TAG = "app_main";

/**
 * @brief Loop principal
 *
 * Nao deve retornar.
 *
 */
void app_main(void) {

  ESP_LOGI(s_TAG, "Executando buzzer_exemplo.c\n"
                  "Confirme que o jumper de ativacao do buzzer "
                  "encontra-se instalado");

  /* Inicia o driver do buzzer */
  if (BuzzerInit() != ESP_OK) {
    ESP_LOGE(s_TAG, "Erro ao inicializar o driver do buzzer...\n");
    for(;;){}
  }

  /* Loop infinito da aplicacao */
  for (;;) {
    /* Ativa o buzzer por 0.5s e desativa por 1.5s
     * Repete por 4 vezes */
    for (int i = 0; i < 4; i++){
      BuzzerSet(1);
      vTaskDelay(500 / portTICK_PERIOD_MS);

      BuzzerSet(0);
      vTaskDelay(1500 / portTICK_PERIOD_MS);
    }

    /* Configura buzzer no modo beat com periodo de 0.6s
     * e duty_cycle de 40%. 
     *
     * Nesse modo nao precisamos nos preocupar em ligar e 
     * desligar o buzzer via BuzzerSet() */
    BuzzerBeat(300, 50);

    /* Aguarda 8 repeticoes do modo beat */
    vTaskDelay(300 * 8 / portTICK_PERIOD_MS);

    /* Desativa o modo beat */
    BuzzerSet(0);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  /* Nao deve chegar aqui!! */
  return;
}
