/*=====================================================================================
 * Description:
 *  // Заглушки функций обработки
 *
 *
 *====================================================================================*/
#ifndef _STAFF_H_
#define _STAFF_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

int staff(const uint8_t* input, size_t len, uint8_t* output);

#ifdef __cplusplus
}
#endif

#endif // _STAFF_H_