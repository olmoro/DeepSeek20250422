/*
 * На данный момент сообщений об ошибках нет. Данные неизвестны.
 *
*/

#include "uart2_task.h"
#include "board.h"
#include "destaff.h"
#include "project_config.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h> // for standard int types definition
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "mb_crc.h"
#include "sp_crc.h"

static const char* TAG = "UART2 Gateway";

static SemaphoreHandle_t uart1_mutex, uart2_mutex;

static SemaphoreHandle_t uart1_mutex = NULL;
static SemaphoreHandle_t uart2_mutex = NULL;

extern uint8_t mb_addr; // адрес MB-slave
extern uint8_t mb_comm; // команда ( MB-функция)

extern uint8_t sp_request;  // 0x86  адрес sp-запросчика (предположительно)
extern uint8_t sp_reply;    // 0x00  адрес sp-ответчика (предположительно)
extern uint8_t sp_comm;     // 0x03  команда (sp-функция) (предположительно)

uint8_t error_sp[5];
uint8_t error_sp_len = sizeof(error_sp);

// Генерация MODBUS ошибки
static void generate_error(uint8_t error_code)
{
    error_sp[0] = mb_addr;          // Адрес
    error_sp[1] = mb_comm |= 0x80;  // Функция
    error_sp[2] = error_code;       // Код ошибки

    /* Расчет CRC для ответа */
    uint16_t error_sp_crc = mb_crc16(error_sp, error_sp_len - 2);
    error_sp[3] = error_sp_crc & 0xFF; // 3
    error_sp[4] = error_sp_crc >> 8;   // 4

    ESP_LOGI(TAG, "Error_packet_len (%d bytes):", error_sp_len);
    for (int i = 0; i < error_sp_len; i++)
    {
        printf("%02X ", error_sp[i]);
    }
    printf("\n");
}

// Задача обработки UART2 (спец. протокол)
void uart2_task(void* arg) 
{
    // Создание очереди и мьютекса
    // Настройка очередей UART (если нужно)

    uart1_mutex = xSemaphoreCreateMutex();
    if (!uart1_mutex)
    {
        ESP_LOGE(TAG, "Mutex1 creation failed");
        return;
    }

    uart2_mutex = xSemaphoreCreateMutex();
    if (!uart2_mutex)
    {
        ESP_LOGE(TAG, "Mutex2 creation failed");
        return;
    }

    uint8_t *frame_buffer = NULL;
    uint16_t frame_length = 0;
    uint32_t last_rx_time = 0;    

    while(1) 
    {
        uint8_t temp_buf[BUF_SIZE]; // уточнить
        uint16_t bytes = 0x00;      // количество байт в содержательной части пакета (сообщение об ошибке)
        bool is_valid = true;

        int len = uart_read_bytes(SP_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(100));

        if(len > 0) 
        {
            // Проверка целостности пакета

            // Начало нового фрейма
            if (frame_buffer == NULL)
            {
                frame_buffer = malloc(MAX_PDU_LENGTH * 2);
                frame_length = 0;
            }

            // Проверка переполнения буфера
            if (frame_length + len > MAX_PDU_LENGTH * 2)
            {
                ESP_LOGE(TAG, "Buffer overflow! Discarding frame");
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            ESP_LOGI(TAG, "len (%d bytes):", len);

            memcpy(frame_buffer + frame_length, temp_buf, len);
            frame_length += len;
            last_rx_time = xTaskGetTickCount();

            ESP_LOGI(TAG, "frame_buffer (%d bytes):", frame_length);
            for (int i = 0; i < frame_length; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");
        }

        // Проверка завершения фрейма
        if (frame_buffer && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(SP_FRAME_TIMEOUT_MS * 4))
        {
            // Минимальная длина фрейма: FF FF 10 01 86 00 10 1F + CRC
            if (frame_length < 10)  // Уточнить
            {
                ESP_LOGE(TAG, "Invalid frame length: %d", frame_length);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            // Проверка адреса - пока без проверки
            // if (frame_buffer[0] != SLAVE_ADDRESS)
            // {
            //     ESP_LOGW(TAG, "Address mismatch: 0x%02X", frame_buffer[0]);
            //     free(frame_buffer);
            //     frame_buffer = NULL;
            //     frame_length = 0;
            //     continue;
            // }

            // Проверка CRC
            // ESP_LOGI(TAG, "frame_buffer (%d bytes):", frame_length);
            // for (int i = 0; i < frame_length; i++)
            // {
            //     printf("%02X ", frame_buffer[i]);
            // }
            // printf("\n");

            uint16_t received_crc = (frame_buffer[frame_length - 2] << 8) | frame_buffer[frame_length - 1];
            uint16_t calculated_crc = sp_crc16(frame_buffer + 4, frame_length - 6);

            if (received_crc != calculated_crc)
            {
                ESP_LOGE(TAG, "CRC error: %04X vs %04X", received_crc, calculated_crc);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            ESP_LOGI(TAG, "SP CRC OK");

            // Из frame_buffer исключаются первые (FF FF) и последние (CRC) байты
            bytes = frame_length - 4;                                               // Пакет будет на 4 байта короче
            memmove(frame_buffer, frame_buffer + 2, bytes);                         // сдвиг на 2 байта

            ESP_LOGI(TAG, "length %d bytes:", bytes);
            for (int i = 0; i < bytes; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");

            // Дестаффинг
            bytes = deStaff(frame_buffer, bytes); 

            ESP_LOGI(TAG, "Destaffing (%d bytes):", bytes);
            for (int i = 0; i < bytes; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");
            
            if (is_valid)
            {
                /* Формирование ответа для UART1 (MB) */
                // bytes: mb_addr mb_comm bytes_h,l <- bytes -> mb-crc_h,l =- bytes + 6
                uint8_t *responce = malloc(bytes + 6);

                responce[0] = mb_addr;          // Адрес;
                responce[1] = mb_comm;          // Функция;
                responce[2] = bytes >> 8;       // bytes_h 
                responce[3] = bytes & 0xFF;     // bytes_l
                memcpy(responce + 4, frame_buffer, bytes);

                // Расчет CRC для uart1
                bytes += 4; // Добавлен заголовок
                // ESP_LOGI(TAG, "responce (%d bytes):", bytes);
                // for (int i = 0; i < bytes; i++)
                // {
                //     printf("%02X ", responce[i]);
                // }
                // printf("\n");

                /* Расчет CRC для ответа */
                uint16_t responce_mb_crc = mb_crc16(responce, bytes);
                responce[bytes + 1] = responce_mb_crc & 0xFF;   // 3
                responce[bytes + 2] = responce_mb_crc >> 8;     // 4

                bytes += 2; // Добавлена CRC

                ESP_LOGI(TAG, "responce (%d bytes):", bytes);
                for (int i = 0; i < bytes; i++)
                {
                    printf("%02X ", responce[i]);
                }
                printf("\n");

                // Отправка если нет ошибки

                xSemaphoreTake(uart1_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char *)responce, bytes);
                xSemaphoreGive(uart1_mutex);
                    // 01 10 00 1F 01 86 00 1F 03 33 33 32 02 09 30 09 30 30 33 0C 09 32 30 36 30 31 30 30 30 30 35 09 20 0C 03 00 7F 
                // Освобождение памяти
                free(responce);

                ledsBlue();
            }
            else
            {
                generate_error(0x04);   // 0x01 - Недопустимая функция
                xSemaphoreTake(uart1_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char *)error_sp, 5 ); //sizeof(&error_sp));
                xSemaphoreGive(uart1_mutex);

                ledsBlue();
            }

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;
        }
    }
}
