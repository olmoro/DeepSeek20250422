/*
 *   Вариант со сдвигами в одном буфере
 *
 */
#include "destaff.h"
#include "project_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

/* Для структурирования сообщений используются управляющие символы */
#define DLE 0x10 // символ-префикс
#define SOH 0x01 // начало заголовка
#define ISI 0x1F // указатель кода функции FNC
#define STX 0x02 // начало тела сообщения
#define ETX 0x03 // конец тела сообщения

/* Использованы следующие обозначения:
DAD  байт адреса приёмника,
SAD  байт адреса источника,
FNC  байт кода функции,
CRC1, CRC2  циклические контрольные коды
*/

static const char *TAG = "DESTAFF";

int deStaff(uint8_t *input, size_t len) // input одновременно и output
{
    // Проверка валидности входных аргументов
    if (input == NULL || len == NULL)
        return 0;

    if (len < BUF_MIN_SIZE || len > BUF_SIZE * 2)
        return 0;

    size_t read_idx = 0;
    size_t write_idx = 0;

    while (read_idx < len)
    {
        // Проверяем текущий и следующий байт, если есть
        if (input[read_idx] == DLE && (read_idx + 1 < len))
        {
            uint8_t next_byte = input[read_idx + 1];

            // Проверяем следующий байт на совпадение с целевыми значениями
            bool should_remove = (next_byte == SOH || next_byte == STX ||
                                  next_byte == ETX || next_byte == ISI);

            if (should_remove)
            {
                // Пропускаем запись текущего байта (DLE)
                read_idx++; // Переходим к следующему байту после DLE
                continue;
            }
        }

        // Копируем текущий байт в новую позицию
        input[write_idx] = input[read_idx];
        write_idx++;
        read_idx++;
    }

    return write_idx;
}
