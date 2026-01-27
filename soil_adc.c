/*
 * soil_adc.c
 *
 *  Created on: Jan 27, 2026
 *      Author: quyen
 */
#include "soil_adc.h"


#define ADC_DRY 3300.0f  // Giá trị khi đất khô (thay đổi theo cảm biến của bạn)
#define ADC_WET 1800.0f  // Giá trị khi đất ướt (thay đổi theo cảm biến của bạn)

static ADC_HandleTypeDef hadc_local;

static void Soil_GPIO_AnalogInit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if (GPIOx == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (GPIOx == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (GPIOx == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (GPIOx == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef gi = {0};
    gi.Pin  = GPIO_Pin;
    gi.Mode = GPIO_MODE_ANALOG;
    gi.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &gi);
}

static void Soil_ADC_Init(SoilADC_t *s)
{
    if (s->ADCx == ADC1) __HAL_RCC_ADC1_CLK_ENABLE();
#ifdef ADC2
    else if (s->ADCx == ADC2) __HAL_RCC_ADC2_CLK_ENABLE();
#endif

    hadc_local.Instance = s->ADCx;
    hadc_local.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc_local.Init.ContinuousConvMode = DISABLE;
    hadc_local.Init.DiscontinuousConvMode = DISABLE;
    hadc_local.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc_local.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc_local.Init.NbrOfConversion = 1;

    if (HAL_ADC_Init(&hadc_local) != HAL_OK) { while (1) {} }

    ADC_ChannelConfTypeDef conf = {0};
    conf.Channel = s->channel;
    conf.Rank = ADC_REGULAR_RANK_1;
    conf.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;

    if (HAL_ADC_ConfigChannel(&hadc_local, &conf) != HAL_OK) { while (1) {} }

#if defined(ADC_CR2_CAL)
    HAL_ADCEx_Calibration_Start(&hadc_local);
#endif
}

void Soil_Begin(SoilADC_t *s)
{
    Soil_GPIO_AnalogInit(s->GPIOx, s->GPIO_Pin);
    Soil_ADC_Init(s);
}

float Soil_ReadRaw(SoilADC_t *s)
{
    (void)s;
    HAL_ADC_Start(&hadc_local);
    HAL_ADC_PollForConversion(&hadc_local, 10);
    uint16_t v = (uint16_t)HAL_ADC_GetValue(&hadc_local);
    HAL_ADC_Stop(&hadc_local);
    /* Quy đổi ADC sang độ ẩm đất */
      float tu_so  = (float)v - (float)ADC_DRY;
     float  mau_so = (float)ADC_WET - (float)ADC_DRY;
      float hum = (tu_so / mau_so) * 100.0f;

       /* Giới hạn giá trị */
       if (hum > 100.0f) hum = 100.0f;
       if (hum < 0.0f)   hum = 0.0f;
    return hum;
}


