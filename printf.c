/*
 * printf.c
 *
 *  Created on: Jan 12, 2026
 *      Author: quyen
 */


#include "printf.h"

void myPrintf(UART_HandleTypeDef *huart, const char *format, ...)
{
    char buff[100];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buff, sizeof(buff), format, args);
    va_end(args);
    if (len > 0)
    {
        if (len > sizeof(buff))
            len = sizeof(buff);
        HAL_UART_Transmit(huart, (uint8_t *)buff, len, 10);
    }
}
