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
 * @file include/oled_driver.h
 *
 * @brief Interface para interagir com o display OLED.
 *
 * @author Vinicius Silva <silva.viniciusr@gmail.com>
 *
 * @date 03 de Julho de 2025
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Solicita inicializacao do driver do OLED.
 *
 * @return
 *    - ESP_OK (0): Success.
 *    - ESP_FAIL: Falha ao inicilizar o HW do display.
 *    - ESP_ERR_NOT_ALLOWED: Driver ja encontra-se inicializado.
 *
 */
esp_err_t OledInit(void);

/**
 * @brief Escreve bitmap no display.
 * 0, 0 (origem) corresponde ao topo esquerdo do display.
 *
 * @x_size   : quantidade de bits na direcao horizontal.
 * @y_size   : quantidade de bits na direcao vertical.
 * @x_offset : offset horizontal para imagem.
 * @y_offset : offset vertical para imagem.
 * @bitmap   : array com representacao da imagem 1px por bit.
 *
 * @return
 *    - ESP_OK (0): Success.
 *    - ESP_ERR_INVALID_ARG:   Parametros incorretos.
 *    - ESP_ERR_INVALID_STATE: Driver nao inicializado.
 *
 */
esp_err_t OledDrawBitmap(int x_size, int y_size, int x_offset, int y_offset, const uint8_t *bitmap);

