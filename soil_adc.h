/*
 * soil_adc.h
 *
 *  Created on: Jan 27, 2026
 *      Author: quyen
 */

#ifndef INC_SOIL_ADC_H_
#define INC_SOIL_ADC_H_

#include "stm32f1xx_hal.h"

typedef struct
{
    ADC_TypeDef   *ADCx;      // VD: ADC1
    uint32_t       channel;   // VD: ADC_CHANNEL_2 (PA2)

    GPIO_TypeDef  *GPIOx;     // VD: GPIOA
    uint16_t       GPIO_Pin;  // VD: GPIO_PIN_2
} SoilADC_t;

void     Soil_Begin(SoilADC_t *s);
float Soil_ReadRaw(SoilADC_t *s);




#endif /* INC_SOIL_ADC_H_ */
