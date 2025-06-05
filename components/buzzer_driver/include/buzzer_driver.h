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
 * @file include/buzzer_driver.h
 *
 * @brief Interface para interagir com o buzzer.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 29 de Maio de 2025
 */

#pragma once

#include "esp_err.h"

/**
 * @brief Solicita inicializacao do driver do buzzer.
 *
 * @return
 *    - ESP_OK (0): Success.
 *    - ESP_FAIL: Falha ao criar a tarefa BUZZER_D.
 *    - ESP_ERR_NOT_ALLOWED: Driver ja encontra-se inicializado.
 *
 */
esp_err_t BuzzerInit(void);

/**
 * @brief Liga e desliga o buzzer.
 *
 * @param value: 0 = OFF
 *               1 = ON
 *
 * @return
 *    - ESP_OK (0): Success.
 *    - ESP_ERR_INVALID_STATE: Driver nao inicializado.
 *
 */
esp_err_t BuzzerSet(char value);

/**
 * @brief Configura o buzzer para tocar periodicamente com determinado duty cycle.
 *
 * @param period Periodo em MS
 * @param duty_cycle Duty cycle 0-100
 *
 * caso period = 0 o buzzer sera desligado.
 * caso duty_cycle > 100, sera truncado em 100.
 *
 * @return
 *    - ESP_OK (0): Success.
 *    - ESP_ERR_INVALID_STATE: Driver nao inicializado.
 *
 */
esp_err_t BuzzerBeat(unsigned period, unsigned duty_cycle);
