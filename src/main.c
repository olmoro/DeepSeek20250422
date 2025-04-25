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

static const char *TAG = "UART Gateway";
// static QueueHandle_t uart1_queue, uart2_queue;
// static SemaphoreHandle_t uart1_mutex, uart2_mutex;

void app_main(void)
{
    // Инициализация периферии
    boardInit();
    uart_mb_init();
    uart_sp_init();

    // // Инициализация примитивов синхронизации
    // uart1_mutex = xSemaphoreCreateMutex();
    // uart2_mutex = xSemaphoreCreateMutex();

    // // Настройка очередей UART (если нужно)
    // uart_driver_install(MB_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 10, &uart1_queue, 0);
    // uart_driver_install(SP_PORT_NUM, UART_BUF_SIZE, UART_BUF_SIZE, 10, &uart2_queue, 0);

    // Создание задач
    xTaskCreate(uart1_task, "UART1 Task", 4096, NULL, 5, NULL);
    xTaskCreate(uart2_task, "UART2 Task", 4096, NULL, 5, NULL);     // 6

    ESP_LOGI(TAG, "System initialized");

    /* Проверка RGB светодиода */
    ledsBlue();
}
