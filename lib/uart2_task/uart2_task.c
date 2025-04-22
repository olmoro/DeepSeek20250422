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


static const char* TAG = "UART2 Gateway";
//static QueueHandle_t uart1_queue, uart2_queue;
static SemaphoreHandle_t uart1_mutex;       //, uart2_mutex;



// Задача обработки UART2 (спец. протокол)
void uart2_task(void* arg) 
{
    uint8_t* packet = malloc(SP_MAX_LEN);
    uint8_t* response = malloc(MB_MAX_LEN);
    
    while(1) 
    {
        int len = uart_read_bytes(SP_PORT_NUM, packet, SP_MAX_LEN, pdMS_TO_TICKS(100));
        
        if(len > 0) 
        {
            uint8_t* processed = malloc(MB_MAX_LEN);
            int processed_len = deStuff(packet, len, processed);
            
            if
            (processed_len > 0) {
                // Упаковка в MODBUS (добавьте реальную упаковку)
                xSemaphoreTake(uart1_mutex, portMAX_DELAY);
                uart_write_bytes(MB_PORT_NUM, (const char*)processed, processed_len);
                xSemaphoreGive(uart1_mutex);
            }
            free(processed);
        }
    }
    free(packet);
    free(response);
}
