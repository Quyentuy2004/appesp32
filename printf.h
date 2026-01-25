/*
 * printf.h
 *
 *  Created on: Jan 12, 2026
 *      Author: quyen
 */

#ifndef INC_PRINTF_H_
#define INC_PRINTF_H_
#include <stdio.h>
#include <stdarg.h>
#include "stm32f1xx_hal.h"
void myPrintf(UART_HandleTypeDef *huart,const char*format,...);


#endif /* INC_PRINTF_H_ */
