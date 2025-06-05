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
 * @file dht11_driver.c
 *
 * @brief Driver para o sensor DHT11.
 *
 * Tarefa e responsavel pela ativacao da leitura e composicao do resultado.
 * Comunicacao com o sensor via interrupcao de GPIO.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 14 de Maio de 2025
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht11_driver.h"

#define DHT11_BYTES                     5
#define DHT11_TRIGGER_DELAY_MS          20
#define DHT11_TIMEOUT_US                5000

static const char *s_TAG = "DHT11_D";

/* Datasheet do sensor:
 * https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf
 */

struct driver_state {
/* Contador de bits recebidos */
  int bit_count;

/* Bit stream recebido do sensor */
  uint64_t bitstream;

/* Bytes recebidos do sensor */
  unsigned char bytes[DHT11_BYTES];

/* Variaves para se calcular delta de tempo entra os bits recebidos */
  int64_t current_time;
  int64_t previous_time;

/* Handle da tarefa para que se possa disparar a leitura do sensor */
  TaskHandle_t task_handle;

/* Handle de timer para configuracao do timeout da leitura do sensor */
  esp_timer_handle_t timer_handle;

/* Variavel para indicar se a leitura esta pronta */
  unsigned char data_ready;

/* Variavel para verificar erro */
  esp_err_t rc;
};

static struct driver_state *s_d_statep = NULL;

/**
 * @brief Callback para timout do timer.
 *
 * Callback indica possivel erro ou fim de leitura.
 * Verificar d_state.bit_count.
 *
 * @param arg ponteiro de parametros para callback
 * Nao utilizado. arg = NULL
 *
 */
static void s_TimeoutCallback(void *arg);

/**
 * @brief Callback de interupcao do GPIO.
 *
 * Callback para ler bit envidado pelo sensor DHT11.
 *
 * @param arg ponteiro para dados.
 * Nao utilizado. arg = NULL
 *
 */
static void s_GpioRecvData(void *arg);

/**
 * @brief Inicializacao interna do driver DHT11.
 *
 * Configura o pino GPIO do sensor e prepara o timer para indicar
 * final de leitura.
 *
 * @return
 *    - ESP_OK (0): Success
 *    - Negative value: Error
 *
 */
static esp_err_t s_Dht11Init(void);

/**
 * @brief Loop principal da tarefa DHT11
 *
 * Nao deve retornar.
 *
 * @param pvParameters ponteiro para dados.
 * Nao utilizado. arg = NULL
 *
 */
static void s_Dht11Task(void *pvParameters) {

  struct driver_state d_state;
  s_d_statep = &d_state;

  if (s_Dht11Init()) {
    ESP_LOGE(s_TAG, "Erro durante a initializacao do driver.\n"
                    "error code: %d \n", d_state.rc);
    s_d_statep = NULL;
    vTaskDelete(NULL);
    return;
  }

  for (;;) {
    vTaskSuspend(NULL); //aguarda solicitacao de nova leitura.
    d_state.data_ready = 0;

    //Configura CONFIG_DHT11_GPIO para zero por > 20ms para disparar uma nova leitura.
    if (gpio_set_level(CONFIG_DHT11_GPIO, 0)) {
      d_state.rc = -1; //gpio_error
      goto driver_error;
    }
    vTaskDelay(DHT11_TRIGGER_DELAY_MS / portTICK_PERIOD_MS);

    d_state.bit_count = 0;
    d_state.bitstream = 0;
    d_state.previous_time = esp_timer_get_time();

    //Dispara o timer com timeout de leitura e libera o GPIO para o sensor.
    if (esp_timer_start_once(d_state.timer_handle, DHT11_TIMEOUT_US)) {
      d_state.rc = -2; //timer_error
      goto driver_error;
    }

    if (gpio_set_level(CONFIG_DHT11_GPIO, 1)) {
      d_state.rc = -1; //gpio_error
      goto driver_error;
    }

    vTaskSuspend(NULL); //aguarda leitura completar

    if (d_state.bit_count < 42) { //erro de leitura
      ESP_LOGE(s_TAG, "Erro durante a leitura do sensor... Tente novamente...\n");
      memset(d_state.bytes, 0, sizeof(d_state.bytes));
      d_state.data_ready = 1;
      continue;
    }

    //Popula os bits recebidos em d_state.bytes[]
    for (int i = 0; i < DHT11_BYTES; i++) {
       d_state.bytes[i] = (d_state.bitstream >> (32 - 8 * i)) & 0xff;
    }

    if (d_state.bytes[4] != d_state.bytes[0] + d_state.bytes[1] +
                         d_state.bytes[2] + d_state.bytes[3]) {
      ESP_LOGW(s_TAG, "Warning! Checksum incorreto!\n");
    }

    d_state.data_ready = 1;
  }

//Nao deveria chegar aqui, a nao ser com erro.
driver_error:
  switch (d_state.rc) {
    case -1:
      ESP_LOGE(s_TAG, "Erro durante set do GPIO.\n");
      break;
    case -2:
      ESP_LOGE(s_TAG, "Erro durante set do timer.\n");
      break;
    default:
      ESP_LOGE(s_TAG, "Driver error.\n");
  }

  ESP_LOGE(s_TAG, "Deletando a tarefa %s...\n", s_TAG);
  s_d_statep = NULL;
  vTaskDelete(NULL);

  return;
}

esp_err_t s_Dht11Init(void) {

  s_d_statep->rc = ESP_OK;
  memset(s_d_statep->bytes, 0, sizeof(s_d_statep->bytes));
  s_d_statep->data_ready = 1;

  //Para trigar nova medicao ou retomar execucao da tarefa apos medicao
  s_d_statep->task_handle = xTaskGetCurrentTaskHandle();

  //Configuracao do timer de timeout de medicao
  const esp_timer_create_args_t timer_param = {
    .callback = s_TimeoutCallback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "DHT11",
    .skip_unhandled_events = true
  };

  const  gpio_config_t gpio_handle = {
    .pin_bit_mask = 1LLU << CONFIG_DHT11_GPIO,
    .mode = GPIO_MODE_INPUT_OUTPUT_OD,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE
  };

  if ((s_d_statep->rc = esp_timer_create(&timer_param, &(s_d_statep->timer_handle))))
    return s_d_statep->rc = -1;

  if ((s_d_statep->rc= gpio_config(&gpio_handle)))
    return s_d_statep->rc = -2;

  if((s_d_statep->rc = gpio_set_level(CONFIG_DHT11_GPIO, 1)))
    return s_d_statep->rc = -3;

  if ((s_d_statep->rc = gpio_install_isr_service(ESP_INTR_FLAG_LOWMED)))
    return s_d_statep->rc = -4;

  if ((s_d_statep->rc = gpio_isr_handler_add(CONFIG_DHT11_GPIO, s_GpioRecvData, NULL)))
    return s_d_statep->rc = -5;

  return s_d_statep->rc;
}


void s_GpioRecvData(void *arg) {

  s_d_statep->current_time = esp_timer_get_time();
  esp_timer_restart(s_d_statep->timer_handle, DHT11_TIMEOUT_US);
  s_d_statep->bitstream <<= 1;
  s_d_statep->bitstream += (s_d_statep->current_time - s_d_statep->previous_time < 100) ? 0 : 1;
  s_d_statep->bit_count++;
  s_d_statep->previous_time = s_d_statep->current_time;
}


void s_TimeoutCallback(void *arg) {

  vTaskResume(s_d_statep->task_handle);
}

/* Init publico do driver DHT11 */
esp_err_t Dht11Init() {
  if (s_d_statep) {
    return ESP_ERR_NOT_ALLOWED;
  }

  /*  Registra a tarefa DHT11 */
  if (xTaskCreate(s_Dht11Task, s_TAG, CONFIG_DHT11_TASK_STACK_SIZE, NULL,
                  CONFIG_DHT11_TASK_PRIORITY, NULL) != pdPASS) {
    ESP_LOGE(s_TAG, "Erro criando a tarefa %s...\n", s_TAG);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t Dht11Update(void) {
  int64_t current_time;
  if (s_d_statep) {
    current_time = esp_timer_get_time();
    /* Pelo menos 1 segundo de intervalo entre as medicoes */
    if ((current_time - s_d_statep->previous_time) > 1000 * 1000) {
      vTaskResume(s_d_statep->task_handle);
    }
    return ESP_OK;
  }
  return ESP_ERR_INVALID_STATE;
}

esp_err_t Dht11Read(dht11_data_t *dht11_data) {
  if (s_d_statep) {
    if (s_d_statep->data_ready && dht11_data) {
      //Converte ponto fixo em ponto flutuante...
      float temp = s_d_statep->bytes[0];
      dht11_data->relative_humidity = temp;
      int i = 10;
      while (s_d_statep->bytes[1] / i) {
        i = i * 10;
      }
      temp = i;
      temp = s_d_statep->bytes[1] / temp;
      dht11_data->relative_humidity += temp;

      temp = s_d_statep->bytes[2];
      dht11_data->temperature = temp;
      i = 10;
      while (s_d_statep->bytes[3] / i) {
        i = i * 10;
      }
      temp = i;
      temp = s_d_statep->bytes[3] / temp;
      dht11_data->temperature += temp;

      return ESP_OK;
    }
    return ESP_ERR_NOT_FINISHED;
  }
  return ESP_ERR_INVALID_STATE;
}

