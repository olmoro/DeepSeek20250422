/*
   ---------------------------------------------------------------------------------
                            Файл конфигурации проекта
   ---------------------------------------------------------------------------------
*/

#pragma once

#include <stdint.h>
#include "esp_task.h"

#include <time.h>
#include <stdio.h>
// #include <stdint.h>
#include <stdbool.h>
#include "esp_bit_defs.h"
// ---------------------------------------------------------------------------------
//                                  Версии
// ---------------------------------------------------------------------------------
#define APP_VERSION "MB_SP_00 20250422.05"
// 202500422.05:  add: Test  115200 OK                      RAM:  3.5%  Flash: 13.2%
// 202500422.04:  add: Test OK                              RAM:  3.5%  Flash: 13.1%
// 202500422.02:  add: uart1_task OK                        RAM:  3.5%  Flash: 13.1%
// 202500422.01:  add: mb_crc sp_crc                        RAM:  3.5%  Flash: 13.0%
// 202500422.00:  add:                                      RAM:  3.5%  Flash: 13.3%

// modbus -> 01 10 00 02 00 0A 14 01 00 86 1F 1D 33 33 32 02 09 30 30 30 09 30 30 33 0C 03 00 59 2D
// sp     -> 10 01 00 86 10 1F 1D 33 33 32 10 02 09 30 30 30 09 30 30 33 0C 10 03 42 16
// reply  <- FF FF 10 01 86 00 10 1F 03 33 33 32 10 02 09 30 09 30 30 33 0C 09 32 30 36 30 31 30 30 30 30 35 09 20 0C 10 03 32 61


#define TEST_MODBUS

// ---------------------------------------------------------------------------------
//                                  GPIO
// ---------------------------------------------------------------------------------
// Плата SPN.55
// Светодиоды
#define RGB_RED_GPIO 4   // Красный, катод на GND (7mA)
#define RGB_GREEN_GPIO 2 // Зелёный, катод на GND (5mA)
#define RGB_BLUE_GPIO 27 // Синий,   катод на GND (4mA)

// Входы
#define CONFIG_GPIO_IR 19 // Вход ИК датчика

// UART1
#define CONFIG_MB_UART_RXD 25
#define CONFIG_MB_UART_TXD 26
#define CONFIG_MB_UART_RTS 33

// UART2
#define CONFIG_SP_UART_RXD 21
#define CONFIG_SP_UART_TXD 23
#define CONFIG_SP_UART_RTS 22

#define CONFIG_UART_DTS 32 // Не используется

#define A_FLAG_GPIO        17
#define B_FLAG_GPIO        16


// ---------------------------------------------------------------------------------
//                                    Общие
// ---------------------------------------------------------------------------------
#define BUF_SIZE (240) // размер буфера
#define BUF_MIN_SIZE (4) // минимальный размер буфера
#define MAX_PDU_LENGTH 240


// ---------------------------------------------------------------------------------
//                                    MODBUS
// ---------------------------------------------------------------------------------

// #define PRB // Тестовый. Их принятого фрейма исключается вся служебная информация

#define MB_PORT_NUM UART_NUM_1
#define MB_BAUD_RATE 9600
#define SLAVE_ADDRESS 0x01 //+ 1
#define MB_QUEUE_SIZE 2
#define MB_FRAME_TIMEOUT_MS 4 // 3.5 символа при 19200 бод


#define MB_MAX_LEN       250
#define SP_MAX_LEN       MB_MAX_LEN * 2
#define UART_BUF_SIZE    (1024 * 2)
#define QUEUE_SIZE       10
#define RESP_TIMEOUT_MS  100


// ---------------------------------------------------------------------------------
//                                    SP
// ---------------------------------------------------------------------------------

#define SP_PORT_NUM UART_NUM_2
#define SP_BAUD_RATE 115200      //9600

#define SP_ADDRESS 0x00
#define SP_QUEUE_SIZE 2
#define SP_FRAME_TIMEOUT_MS 10   // По факту

// Константы протокола
#define SOH 0x01        // Байт начала заголовка
#define ISI 0x1F        // Байт указателя кода функции
#define STX 0x02        // Байт начала тела сообщения
#define ETX 0x03        // Байт конца тела сообщения
#define DLE 0x10        // Байт символа-префикса
#define CRC_INIT 0x1021 // Битовая маска полинома
// #define FRAME 128       //

// ---------------------------------------------------------------------------------
//                                    Задачи (предварительно)
// ---------------------------------------------------------------------------------

// // Размер стеков
// #define CONFIG_MB_TASK_STACK_SIZE 1024 * 4
// #define CONFIG_PR_TASK_STACK_SIZE 1024 * 4

// // Приоритеты
// #define CONFIG_MB_RECEIVE_TASK_PRIORITY 5 // modbus_receive_task
// #define CONFIG_PROCESSING_TASK_PRIORITY 4 // processing_task
