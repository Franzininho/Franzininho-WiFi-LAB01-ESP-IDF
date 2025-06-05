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
 * @file buzzer_driver.c
 *
 * @brief Driver para buzzer.
 *
 * Tarefa e responsavel pela configuracao, ativacao e desativacao do buzzer.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 29 de Maio de 2025
 */

#include <stdio.h>
#include "esp_log.h"
#include "driver/dac_cosine.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer_driver.h"

#ifdef CONFIG_BUZZER_ATTENUATION_DB_0
#define BUZZER_ATTENUATION DAC_COSINE_ATTEN_DB_0
#elifdef CONFIG_BUZZER_ATTENUATION_DB_6
#define BUZZER_ATTENUATION DAC_COSINE_ATTEN_DB_6
#elifdef CONFIG_BUZZER_ATTENUATION_DB_12
#define BUZZER_ATTENUATION DAC_COSINE_ATTEN_DB_12
#endif

static const char *s_TAG = "BUZZER_D";

struct driver_state {

/* Handle da tarefa para que se possa disparar modo beat */
  TaskHandle_t task_handle;

/* Ativa modo beat */
  char beat;

/* Periodo em MS para tocar o buzzer (BuzzerBeat) */
  unsigned period_on;

/* Periodo em MS para desligar o buzzer (BuzzerBeat) */
  unsigned period_off;

/* Handle para output de onda cossenoidal no GPIO_17 -> DAC_0 */
  dac_cosine_handle_t dac0_handle;

/* Variavel para verificar possivel erro */
  esp_err_t rc;
};

static struct driver_state *s_d_statep = NULL;

/**
 * @brief Inicializacao interna do driver do buzzer.
 *
 * Configura DAC cossenoidal no GPIO do buzzer.
 *
 * @return
 *    - ESP_OK (0): Success
 *    - Negative value: Error
 *
 */
static esp_err_t s_BuzzerInit(void);

/**
 * @brief Loop principal da tarefa BUZZER_D
 *
 * Nao deve retornar.
 *
 * @param pvParameters ponteiro para dados.
 * Nao utilizado. arg = NULL
 *
 */
static void s_BuzzerTask(void *pvParameters) {

  struct driver_state d_state;
  s_d_statep = &d_state;

  if (s_BuzzerInit()) {
    ESP_LOGE(s_TAG, "Erro durante a initializacao do driver.\n"
                    "error code: %d \n", d_state.rc);
    s_d_statep = NULL;
    vTaskDelete(NULL);
    return;
  }
  vTaskSuspend(NULL);

  /* Loop principal */
  /* Implementa buzzer beat */
  for (;;) {
    if (d_state.beat) {
      //Ativa DAC cossenoidal por period_on ms.
      dac_cosine_start(d_state.dac0_handle);
      vTaskDelay((TickType_t) d_state.period_on / portTICK_PERIOD_MS);
    }
    else
      vTaskSuspend(NULL);
    if (d_state.beat) {
      //Desliga dac cossenoidal por period_off ms.
      dac_cosine_stop(d_state.dac0_handle);
      vTaskDelay((TickType_t) d_state.period_off / portTICK_PERIOD_MS);
    }
    else
      vTaskSuspend(NULL);
  }

  //Nao deveria chegar aqui...
  return;
}

esp_err_t s_BuzzerInit(void) {

  s_d_statep->rc = ESP_OK;

  //Para suspender ou retomar loop principal
  s_d_statep->task_handle = xTaskGetCurrentTaskHandle();

  //Desativa modo beat
  s_d_statep->beat = 0;

  dac_cosine_config_t cos0_cfg = {
        .chan_id = DAC_CHAN_0,
        .freq_hz = CONFIG_BUZZER_FREQUENCY,
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .offset = 0,
        .phase = DAC_COSINE_PHASE_0,
        .atten = BUZZER_ATTENUATION,
        .flags.force_set_freq = true,
  };

  s_d_statep->rc = dac_cosine_new_channel(&cos0_cfg, &(s_d_statep->dac0_handle));

  return s_d_statep->rc;
}


/* Init publico do driver do buzzer.
 * Apenas registra a tarefa no sistema, a inicializacao
 * do dac_0 e realizada no init privado. */
esp_err_t BuzzerInit() {
  if (s_d_statep) {
    return ESP_ERR_NOT_ALLOWED;
  }

  /*  Registra a tarefa BUZZER_D */
  if (xTaskCreate(s_BuzzerTask, s_TAG, CONFIG_BUZZER_TASK_STACK_SIZE, NULL,
                  CONFIG_BUZZER_TASK_PRIORITY, NULL) != pdPASS) {
    ESP_LOGE(s_TAG, "Erro criando a tarefa %s...\n", s_TAG);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t BuzzerSet(char value) {
  if (s_d_statep) {
    s_d_statep->beat = 0;
    if (value) {
      dac_cosine_start(s_d_statep->dac0_handle);
    }
    else {
      dac_cosine_stop(s_d_statep->dac0_handle);
    }
    return ESP_OK;

  }
  return ESP_ERR_INVALID_STATE;
}

esp_err_t BuzzerBeat(unsigned period, unsigned duty_cycle) {
  if (s_d_statep) {
    s_d_statep->period_on = period * duty_cycle / 100;
    s_d_statep->period_off = period - s_d_statep->period_on;
    s_d_statep->beat = 1;
    vTaskResume(s_d_statep->task_handle);
    return ESP_OK;
  }
  return ESP_ERR_INVALID_STATE;
}

