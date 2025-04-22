#include "staff.h"
#include "project_config.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"






// Заглушки функций обработки
int staff(const uint8_t* input, size_t len, uint8_t* output) {
    // Реальная обработка здесь
    memcpy(output, input, len);
    return len;
}
