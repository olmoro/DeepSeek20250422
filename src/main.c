#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "project_config.h"
#include "board.h"
#include "uart1_task.h"
#include "uart2_task.h"





//#define MB_PORT_NUM      UART_NUM_1
//#define SP_PORT_NUM      UART_NUM_2
//#define MB_MAX_LEN       250
//#define SP_MAX_LEN       500
// #define UART_BUF_SIZE    (1024 * 2)
// #define QUEUE_SIZE       10
// #define RESP_TIMEOUT_MS  100

static const char* TAG = "UART Gateway";
static QueueHandle_t uart1_queue, uart2_queue;
static SemaphoreHandle_t uart1_mutex, uart2_mutex;

// // Заглушки функций обработки
// int staff(const uint8_t* input, size_t len, uint8_t* output) {
//     // Реальная обработка здесь
//     memcpy(output, input, len);
//     return len;
// }

// int deStuff(const uint8_t* input, size_t len, uint8_t* output) {
//     // Реальная обработка здесь
//     memcpy(output, input, len);
//     return len;
// }

// // Генерация MODBUS ошибки
// void generate_error(uint8_t* buffer, uint8_t error_code) {
//     // Реальная реализация MODBUS ошибки
//     buffer[0] = 0xFF; // Пример заголовка
//     buffer[1] = error_code;
// }



void app_main(void) 
{
    // Инициализация периферии
    boardInit();
    uart_mb_init();
    uart_sp_init();

    /* Проверка RGB светодиода */
    ledsBlue();

    // // Инициализация примитивов синхронизации
    // uart1_mutex = xSemaphoreCreateMutex();
    // uart2_mutex = xSemaphoreCreateMutex();
    
    // // Настройка очередей UART (если нужно)
    // uart_driver_install(MB_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 10, &uart1_queue, 0);
    // uart_driver_install(SP_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 10, &uart2_queue, 0);
    
    // Создание задач
    xTaskCreate(uart1_task, "UART1 Task", 4096, NULL, 5, NULL);
    xTaskCreate(uart2_task, "UART2 Task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "System initialized");
}
