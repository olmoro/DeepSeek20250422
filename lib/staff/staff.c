#include "staff.h"
#include "project_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

/*
 - заголовок    | SOH | DAD | SAD | ISI | FNC | DataHead |
 или безадресный заголовок  | SOH | ISI | FNC | DataHead |
 - данные                                                | STX | DataSet | ETX |
 - контрольная информация                                                      | CRC1 | CRC2 |
 */

//  #include "staff.h"
//  #include "project_config.h"
//  #include <stdint.h>
//  #include <stdio.h>
//  #include <string.h>
//  #include <stdlib.h>
//  #include "esp_system.h"
//  #include "esp_err.h"
//  #include "esp_log.h"
 
 
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
  
 static const char *TAG = "STAFF";




// // Заглушки функций обработки
// int staff(const uint8_t* input, size_t len, uint8_t* output) {
//     // Реальная обработка здесь
//     memcpy(output, input, len);
//     return len;
// }

//int staff(const uint8_t *input, uint8_t *output, size_t input_len, size_t output_max_len, size_t *output_actual_len)
int staff(const uint8_t *input, size_t input_len, uint8_t *output, size_t output_max_len)
{
    size_t j = 0;
    for (size_t i = 0; i < input_len; i++)
    {
        // Определяем, сколько байт нужно записать
        size_t required = (input[i] == SOH || input[i] == ISI || input[i] == STX || input[i] == ETX) ? 2 : 1;

        // Проверяем, достаточно ли места в целевом буфере
        if (j + required > output_max_len)
        {
  //          *output_actual_len = j; // Текущая длина на момент ошибки
            return 0;
        }

        // Вставляем байты согласно условию
        switch (input[i])
        {

        case SOH:
            output[j++] = DLE;
            output[j++] = SOH;
            break;
        case ISI:
            output[j++] = DLE;
            output[j++] = ISI;
            break;
        case STX:
            output[j++] = DLE;
            output[j++] = STX;
            break;
        case ETX:
            output[j++] = DLE;
            output[j++] = ETX;
            break;
        default:
            output[j++] = input[i];
        }
    }
//    *output_actual_len = j;

    ESP_LOGI(TAG, "Staffing (%d bytes):", j);
    for (int i = 0; i < j; i++)
    {
        printf("%02X ", output[i]);
    }
    printf("\n");

    return j;
}
