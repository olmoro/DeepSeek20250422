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

static const char *TAG = "UART1 Gateway";

static SemaphoreHandle_t uart1_mutex, uart2_mutex;

static SemaphoreHandle_t uart1_mutex = NULL;
static SemaphoreHandle_t uart2_mutex = NULL;

uint8_t addr = 0x00; // адрес MB-slave
uint8_t comm = 0x00; // команда ( MB-функция)

uint8_t error_packet[5];
uint8_t error_packet_len = sizeof(error_packet);

// Генерация MODBUS ошибки
static void generate_error(uint8_t error_code)
{
    error_packet[0] = addr;         // Адрес
    error_packet[1] = comm |= 0x80; // Функция
    error_packet[2] = error_code;   // Код ошибки

    /* Расчет CRC для ответа */
    uint16_t error_packet_crc = mb_crc16(error_packet, error_packet_len - 2);
    error_packet[3] = error_packet_crc & 0xFF; // 3
    error_packet[4] = error_packet_crc >> 8;   // 4

    ESP_LOGI(TAG, "Error_packet_len (%d bytes):", error_packet_len);
    for (int i = 0; i < error_packet_len; i++)
    {
        printf("%02X ", error_packet[i]);
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

            ESP_LOGI(TAG, "frame_buffer (%d bytes):", frame_length);
            for (int i = 0; i < frame_length; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");
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
                addr = frame_buffer[0];
                comm = frame_buffer[1];
                if (frame_buffer[frame_length] == 0x00) // Это из-за терминала (он оперирует 16-битовым типом)
                    bytes = frame_buffer[6] - 1;

                memmove(frame_buffer, frame_buffer + 7, bytes); // сдвиг на 7 байтов
                break;

                // case 0x : // другая команда

            default:
                break;
            }

            ESP_LOGI(TAG, "length %d bytes:", bytes); //
            for (int i = 0; i < bytes; i++)
            {
                printf("%02X ", frame_buffer[i]);
            }
            printf("\n");

            // Отправка в UART2 или UART1, если ошибка

            if (is_valid)
            {
                ledsGreen();
                uint8_t *processed = malloc(BUF_SIZE * 2);
                //@@@ int staff(const uint8_t *input, size_t input_len, uint8_t *output, size_t output_max_len);

                int processed_len = staff(frame_buffer, bytes, processed, BUF_SIZE * 2);

                if (processed_len > 0)
                {
                    xSemaphoreTake(uart2_mutex, portMAX_DELAY);
                    uart_write_bytes(SP_PORT_NUM, (const char *)processed, processed_len);
                    xSemaphoreGive(uart2_mutex);
                }
                free(processed);
            }
            else
            {
                ledsRed();

                generate_error(0x01); // Недопустимая функция
                xSemaphoreTake(uart1_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char *)error_packet, sizeof(error_packet));
                xSemaphoreGive(uart1_mutex);
            }

            free(frame_buffer);
            frame_buffer = NULL;
            frame_length = 0;
        }
    }
}
