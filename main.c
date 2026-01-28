/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "dht22.h"
#include "i2c-lcd.h"
#include "printf.h"
#include "soil_adc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
DHT22_HandleTypeDef dht22;
DHT22_Data_t dht_data;


TIM_HandleTypeDef htim2;
ADC_HandleTypeDef hadc1;
UART_HandleTypeDef huart1;
SoilADC_t soil_cfg = {// BIEN TRUC LUA CHON CHAN CHO CAM BIEN DO AM DAT
    .ADCx = ADC1,
    .channel = ADC_CHANNEL_0,   // PA2
    .GPIOx = GPIOA,
    .GPIO_Pin = GPIO_PIN_0
};
// donnhiemso2
typedef enum {
    TASK_NONE = 0,
    TASK1,
    TASK2,
    TASK3,
	TASK4,
	TASK5
} TaskID;

#define Q_SIZE 10

typedef struct {
    TaskID buf[Q_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} Queue;

Queue q;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

I2C_HandleTypeDef hi2c1;// KHAI BAO CHAN I2C LCD
#define SLAVE_ADDRESS_LCD 0x4E //KHAI BAO DIA CHI I2C LCD

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint32_t last_run = 0;// BIEN THOI GIAN LUU THOI GIAN CHAY LAN TRUOC
float soil=0.0;// BIEN LUU TRU CAM BIEN DO AM DAT
int  g_period_ms=2000;// BIEN LUU TRU THOI GIAN THUC HIEN CHU KY
char rxUart;// BIEN NHAN 1 BYTE UART
static uint8_t  uart_rx_ch;
static char     uart_line[128];
static uint16_t uart_line_len = 0;
static volatile uint8_t uart_line_ready = 0;

// donnhiemso2
volatile uint32_t tick_ms = 0;
volatile uint32_t c1 = 0, c2 = 0, c3 = 0, c4=0 , c5=0;
volatile int t1 = 3000, t2 = 1000, t3 = 3000, t4=1000 , t5=100;

#define TICK_MS 1   // TIM3 interrupt mỗi 1ms
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
static void ProcessCmd(char *line);
//static void UART_StartRxIT(void);
void Task_1(void);
void Task_2(void);
void Task_3(void);
void Task_4(void);
void Task_5(void);
void Task_exe(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


// donnhiemso2
uint8_t Q_Pop(Queue *q, TaskID *task)
{
    if (q->count == 0) return 0; // queue empty

    *task = q->buf[q->head];
    q->head = (q->head + 1) % Q_SIZE;
    q->count--;
    return 1;
}
uint8_t Q_Push(Queue *q, TaskID task)
{
    if (q->count >= Q_SIZE) return 0; // queue full

    q->buf[q->tail] = task;
    q->tail = (q->tail + 1) % Q_SIZE;
    q->count++;
    return 1;
}
void Q_Init(Queue *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

	if (htim->Instance == TIM3)
	    {
		tick_ms += TICK_MS;
	     //   c1 += 1; if (c1 >= 2000) { c1 = 0; HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  }// LED;
		    c1 += TICK_MS; if (c1 >= t1) { c1 = 0; Q_Push(&q, TASK1);  }
	        c2 += TICK_MS; if (c2 >= t2) { c2 = 0; Q_Push(&q, TASK2); }
	        c3 += TICK_MS; if (c3 >= t3 ) { c3 = 0; Q_Push(&q, TASK3); }
	        c4 += TICK_MS; if (c4 >= t4) { c4 = 0; Q_Push(&q, TASK4); }
	        c5 += TICK_MS; if (c5 >= t5) { c5 = 0; Q_Push(&q, TASK5); }

	    }
}
// het 2


static void ProcessCmd(char *line)
{
	// donnhiemso2
	    int node = 0;
	    int per  = 0;

	    // Lệnh: SET,<node>,<period_ms>
	    // Ví dụ: SET,2,1000
	    if (sscanf(line, "SET,%d,%d", &node, &per) == 2)
	    {
	        if (node < 1 || node > 5)
	        {
	            myPrintf(&huart1, "ERR node 1..5\r\n");
	            return;
	        }
	        if (per < 50 || per > 600000)   // tùy bạn giới hạn, 50ms là an toàn hơn
	        {
	            myPrintf(&huart1, "ERR period 50..600000(ms)\r\n");
	            return;
	        }

	        __disable_irq();
	        switch(node)
	        {
	            case 1: t1 = per; c1 = 0; break;
	            case 2: t2 = per; c2 = 0; break;
	            case 3: t3 = per; c3 = 0; break;
	            case 4: t4 = per; c4 = 0; break;
	            case 5: t5 = per; c5 = 0; break;
	            default: break;
	        }
	        __enable_irq();

	        myPrintf(&huart1, "OK SET T%d=%dms\r\n", node, per);
	        return;
	    }
	 // het 2


	    // donnhiemso1
//  if (strncmp(line, "PERIOD=", 7) == 0)
//  {
//    uint32_t p = (uint32_t)atoi(line + 7);
//    if (p >= 2000 && p <= 600000)
//    {
//      g_period_ms = p;
//      myPrintf(&huart1," HAL_UART_Transmit... OK:%d\r\n", g_period_ms);
//    }
//    else
//    {
//    	myPrintf(&huart1," SET PERIOD ERROR\r\n");
//    	myPrintf(&huart1,"YEU CAU: PERIOD >= 2000 && PERIOD <= 600000\r\n");
//    }
//  }
//  else if (strcmp(line, "SAVE") == 0)
//  {
//    // ee_data.magic = CFG_MAGIC;
//    // ee_data.period_ms = g_period_ms;
//    // ee_write();
//    // HAL_UART_Transmit(... "SAVED\r\n")
//  }
//  else if (strcmp(line, "LOAD") == 0)
//  {
//    // ee_read();
//    // if (ee_data.magic == CFG_MAGIC) g_period_ms = ee_data.period_ms;
//    // HAL_UART_Transmit(... "LOADED\r\n")
//  }
//  else
//  {
//    // HAL_UART_Transmit(... "UNKNOWN\r\n")
//  }
   //het 1


}

// chung
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    char c = (char)uart_rx_ch;

    if (!uart_line_ready)
    {
      // Kết thúc dòng khi .
      if (c == '.')
      {
        if (uart_line_len > 0)          // tránh nhận dòng rỗng
        {
          uart_line[uart_line_len] = '\0';
          uart_line_ready = 1;
        }
      }
      else
      {
        // Thêm ký tự vào buffer
        if (uart_line_len < (sizeof(uart_line) - 1))
        {
          uart_line[uart_line_len++] = c;
        }
        else
        {
          // Tràn buffer -> reset
          uart_line_len = 0;
        }
      }
    }

    // Nhận tiếp byte tiếp theo
    HAL_UART_Receive_IT(&huart1, &uart_rx_ch, 1);
  }
}

void Task_1(){
	myPrintf(&huart1,"thuc hien task 1\r\n");
	 if (DHT22_Read(&dht22, &dht_data) == 0)   // 0 = OK
		         {
		 myPrintf(&huart1,"DHT22 read success\r\n");

		         }
		         else
		         {
		        	 myPrintf(&huart1,"DHT22 read error\r\n");
		         }
}

void Task_2(){
	myPrintf(&huart1,"thuc hien task 2\r\n");
	soil = Soil_ReadRaw(&soil_cfg);
}

void Task_3(){
	myPrintf(&huart1,"thuc hien task 3\r\n");
	 myPrintf(&huart1,"Temp: %.2f C, Soil: %.2f %%\r\n",
			                    dht_data.Temperature,
			                    soil);
}

void Task_4(){
	myPrintf(&huart1,"thuc hien task 4\r\n");
	char temp_soil[20];
	char period[20];
	sprintf(temp_soil, "T:%.2fC|S:%.1f%%", dht_data.Temperature, soil);
	//donnhiemso1
	//sprintf(period, "P=%.2f (s)",( g_period_ms/1000.0f));
	//donnhiemso2
	sprintf(period, "%.1f%.1f%.1f%.1f%.1f", (t1/1000.0f),(t2/1000.0f), (t3/1000.0f), (t4/1000.0f), (t5/1000.0f));

	lcd_put_cur(0,0);
	lcd_send_string("                "); // 16 spaces
	lcd_put_cur(0,0);
	lcd_send_string(temp_soil);

	lcd_put_cur(1,0);
	lcd_send_string("                ");
	lcd_put_cur(1,0);
	lcd_send_string(period);
}

void Task_5(){
	myPrintf(&huart1,"thuc hien task 5\r\n");
	if (uart_line_ready)
	  {
		myPrintf(&huart1,"vao task DA NHAN DC CHUOI\r\n");
	    uart_line_ready = 0;
	    ProcessCmd(uart_line);  // xử lý lệnh
	    uart_line_len = 0;      // reset để nhận dòng mới
	  }
}
// het chung
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  // donnhiemso2
  Q_Init(&q);
  HAL_TIM_Base_Start_IT(&htim3);



  //hetdonnhiemso2
  // Gán cấu hình cho DHT22
  dht22.GPIOx = GPIOA;
  dht22.GPIO_Pin = GPIO_PIN_1;
  dht22.htim = &htim2;

  DHT22_Init(&dht22);
      // cấu hình ban đầu cho lcd-i2c
  lcd_init();
  lcd_clear();
  lcd_put_cur(0, 0);
  lcd_send_string("Set up");
  // cấu hình con trỏ hàm cac task
   void (*sptt_task_point_array[5])(void) = {Task_1, Task_2, Task_3,Task_4,Task_5};
   // cau hinh ban dau cho uart
   uart_line_len = 0;
   uart_line_ready = 0;
   HAL_UART_Receive_IT(&huart1, &uart_rx_ch, 1);  // đổi huart1 thành huart bạn dùng
   // cau hinh adc soil
    Soil_Begin(&soil_cfg);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // donnhiemso1
//	  if (HAL_GetTick() - last_run >= g_period_ms)
//	 	  {
//	 	      last_run = HAL_GetTick();
//
//	 	      for (int i = 0; i < 5; i++)
//	 	      {
//	 	          sptt_task_point_array[i]();
//	 	      }
//	 	  }
	  //het1


	  // donnhiemso2
	  TaskID task;

	         if (Q_Pop(&q, &task))
	         {
	             switch(task)
	             {
	                 case TASK1: Task_1(); break;
	                 case TASK2: Task_2(); break;
	                 case TASK3: Task_3(); break;
	                 case TASK4: Task_4(); break;
	                 case TASK5: Task_5(); break;
	                 default: break;
	             }
	         }

  }
  // het2
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7199;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 9;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
