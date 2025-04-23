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

static const char* TAG = "UART1 Gateway";
 
//static QueueHandle_t uart1_queue, uart2_queue;
static SemaphoreHandle_t uart1_mutex, uart2_mutex;


static SemaphoreHandle_t uart1_mutex = NULL;
static SemaphoreHandle_t uart2_mutex = NULL;


// Генерация MODBUS ошибки
void generate_error(uint8_t* buffer, uint8_t error_code) 
{
    // Реальная реализация MODBUS ошибки

    buffer[0] = 0xFF; // Пример заголовка
    buffer[1] = error_code;
    //buffer[2] = 0x00;   
    //buffer[3] = 0x00;
}


// Задача обработки UART1 (MODBUS)
void uart1_task(void* arg) 
{
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

    uint8_t* packet = malloc(MB_MAX_LEN);
    uint8_t* response = malloc(MB_MAX_LEN);
    uint8_t error_packet[4] = {0};
    
    while(1) 
    {
        int len = uart_read_bytes(MB_PORT_NUM, packet, MB_MAX_LEN, pdMS_TO_TICKS(100));
        
        if(len > 0) 
        {
            ledsGreen();
            
            // Проверка целостности пакета (добавьте реальную проверку CRC)
            bool is_valid = true;
            
            if(is_valid) 
            {
                uint8_t* processed = malloc(SP_MAX_LEN);

                int processed_len = staff(packet, len, processed);
                
                if(processed_len > 0) 
                {
                    xSemaphoreTake(uart2_mutex, portMAX_DELAY);
                    uart_write_bytes(SP_PORT_NUM, (const char*)processed, processed_len);
                    xSemaphoreGive(uart2_mutex);
                }
                free(processed);
            } 
            else 
            {
                generate_error(error_packet, 0x01); // Недопустимая функция
                xSemaphoreTake(uart1_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char*)error_packet, sizeof(error_packet));
                xSemaphoreGive(uart1_mutex);
            }
        }
    }
    free(packet);
    free(response);
}
