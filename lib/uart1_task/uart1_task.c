#include "uart1_task.h"
#include "board.h"
#include "staff.h"
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

#define PRF

static const char *TAG = "UART1 Gateway";

static SemaphoreHandle_t uart1_mutex, uart2_mutex;

static SemaphoreHandle_t uart1_mutex = NULL;
static SemaphoreHandle_t uart2_mutex = NULL;

uint8_t mb_addr = 0x00;     // адрес mb-slave
uint8_t mb_comm = 0x00;     // команда (mb-функция)

uint8_t sp_request = 0x86;  // адрес sp-запросчика (предположительно)
uint8_t sp_reply = 0x00;    // адрес sp-ответчика (предположительно)
uint8_t sp_comm = 0x03;     // команда (sp-функция) (предположительно)

uint8_t error_mb[5];
uint8_t error_mb_len = sizeof(error_mb);

// Генерация MODBUS ошибки
static void generate_error(uint8_t error_code)
{
    error_mb[0] = mb_addr;         // Адрес
    error_mb[1] = mb_comm |= 0x80; // Функция
    error_mb[2] = error_code;   // Код ошибки

    /* Расчет CRC для ответа */
    uint16_t error_mb_crc = mb_crc16(error_mb, error_mb_len - 2);
    error_mb[3] = error_mb_crc & 0xFF; // 3
    error_mb[4] = error_mb_crc >> 8;   // 4

    ESP_LOGI(TAG, "Error_packet_len (%d bytes):", error_mb_len);
    for (int i = 0; i < error_mb_len; i++)
    {
        printf("%02X ", error_mb[i]);
    }
    printf("\n");
}

// Задача обработки UART1 (MODBUS)
void uart1_task(void *arg)
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

    while (1)
    {
        uint8_t temp_buf[BUF_SIZE]; // уточнить
        uint16_t bytes = 0x00;      // количество байт в содержательной части пакета (сообщение об ошибке)
        bool is_valid = true;

        int len = uart_read_bytes(MB_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(100));
        //int len = uart_read_bytes(MB_PORT_NUM, temp_buf, sizeof(temp_buf), pdMS_TO_TICKS(1000));

        if (len > 0)
        {
            // Проверка целостности пакета (добавьте реальную проверку CRC)

            // Начало нового фрейма
            if (frame_buffer == NULL)
            {
                frame_buffer = malloc(MAX_PDU_LENGTH + 3); // + лишнее
                frame_length = 0;
            }

            // Проверка переполнения буфера
            if (frame_length + len > MAX_PDU_LENGTH + 3)
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
            #ifdef PRF
            ESP_LOGI(TAG, "frame_buffer (%d bytes):", frame_length);
            for (int i = 0; i < frame_length; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");
            #endif
        }

        // Проверка завершения фрейма
        if (frame_buffer && (xTaskGetTickCount() - last_rx_time) > pdMS_TO_TICKS(MB_FRAME_TIMEOUT_MS))
        {
            // Минимальная длина фрейма: адрес + функция + CRC
            if (frame_length < 4)
            {
                ESP_LOGE(TAG, "Invalid frame length: %d", frame_length);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            // Проверка адреса
            if (frame_buffer[0] != SLAVE_ADDRESS)
            {
                ESP_LOGW(TAG, "Address mismatch: 0x%02X", frame_buffer[0]);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            // Проверка CRC
            uint16_t received_crc = (frame_buffer[frame_length - 1] << 8) | frame_buffer[frame_length - 2];
            uint16_t calculated_crc = mb_crc16(frame_buffer, frame_length - 2);

            if (received_crc != calculated_crc)
            {
                ESP_LOGE(TAG, "CRC error: %04X vs %04X", received_crc, calculated_crc);
                free(frame_buffer);
                frame_buffer = NULL;
                frame_length = 0;
                continue;
            }

            ESP_LOGI(TAG, "MB CRC OK");

            switch (frame_buffer[1])
            {
            case 0x10:
                // сохранить адрес и команду для ответа
                mb_addr = frame_buffer[0];
                mb_comm = frame_buffer[1];
                // if (frame_buffer[frame_length] == 0x00) // Это из-за терминала (он оперирует 16-битовым типом)
                //     bytes = frame_buffer[6] - 1;
                bytes = 0x13;
                memmove(frame_buffer, frame_buffer + 7, bytes); // сдвиг на 7 байтов
                break;

                // case 0x : // другая команда

            default:
                break;
            }

            #ifdef PRF
            ESP_LOGI(TAG, "length %d bytes:", bytes); //    = 0
            for (int i = 0; i < bytes; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");
            #endif
            // Отправка в UART2 или UART1, если ошибка

 //   ESP_LOGI(TAG, "1 MB OK?");

            if (is_valid)
            {
                uint8_t *processed = malloc(BUF_SIZE * 2);
                int actual_len = staff(frame_buffer, bytes, processed, BUF_SIZE * 2);
    ESP_LOGI(TAG, "actual_len %d bytes:", actual_len); //
 //   ESP_LOGI(TAG, "2 MB OK?");
            
                // Формирование ответа для UART2 (SP)
                int sp_len = actual_len + 2; // +2 байта для контрольной суммы
    ESP_LOGI(TAG, "sp_len %d bytes:", sp_len);

                // Расчет CRC для uart2 без двух первых байтов
                uint16_t response_crc = sp_crc16(processed + 2, actual_len - 2);
    ESP_LOGI(TAG, "response_crc %04X :", response_crc);

                processed[actual_len + 1] = response_crc & 0xFF;
                processed[actual_len] = response_crc >> 8;

    ESP_LOGI(TAG, "5 MB OK?");
           
                #ifdef PRF
                ESP_LOGI(TAG, "Response for send to SP (%d bytes):", sp_len);
                for (int i = 0; i < sp_len; i++)
                {
                    printf("%02X ", processed[i]);
                }
                printf("\n");
                #endif
                if (sp_len > 0)
                {
                    xSemaphoreTake(uart2_mutex, portMAX_DELAY);
                    uart_write_bytes(SP_PORT_NUM, (const uint8_t *)processed, sp_len);
                    xSemaphoreGive(uart2_mutex);
                }
                free(processed);
            }
            else
            {
                generate_error(0x01);   // 0x01 - Недопустимая функция
                xSemaphoreTake(uart1_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char *)error_mb, sizeof(error_mb));
                xSemaphoreGive(uart1_mutex);

                ledsBlue();
            }

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;
        }
    }
}
