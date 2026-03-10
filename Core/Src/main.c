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
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
    WAIT_INTERVAL,   // wait between measurements
    TRIGGER,         // send ultrasonic trigger pulse
    WAIT_RISING,     // wait for echo signal rising edge
    WAIT_FALLING,    // wait for echo signal falling edge
    DONE,            // measurement complete
    TIMEOUT          // no echo received
} state_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile state_t state = TRIGGER;
volatile uint32_t ic_start = 0;
volatile uint32_t ic_end = 0;
volatile uint32_t timeout_start = 0;
volatile uint32_t last_time = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/**
 * @brief Microsecond busy-wait delay using TIM2.
 *
 * Delays execution for the specified number of microseconds by polling
 * the TIM2 counter register. The function records the current timer
 * value and waits until the elapsed counter difference reaches the
 * requested delay time.
 *
 * The delay relies on TIM2 being configured with a 1 MHz timer clock
 * (1 timer tick = 1 µs). The subtraction-based comparison ensures that
 * the delay works correctly even if the timer counter overflows.
 *
 * Note: This is a blocking delay that keeps the CPU busy while waiting.
 *
 * @param us Delay duration in microseconds.
 */
static void delay_us(uint16_t us);

/**
 * @brief Transmit an unsigned integer over UART without using printf.
 *
 * Converts the given unsigned integer value into its ASCII decimal
 * representation and sends the digits sequentially over UART.
 *
 * The conversion is performed manually by repeatedly extracting the
 * least significant digit using modulo 10 and storing the digits in a
 * temporary buffer in reverse order. The buffer is then transmitted
 * in reverse to restore the correct digit order.
 *
 * This function avoids the use of printf in order to reduce code size
 * and dependencies.
 *
 * @param value Unsigned integer value to be transmitted via UART.
 */
static void uart_send_uint(uint16_t value);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief Program entry point.
 *
 * Initializes the microcontroller hardware and starts the ultrasonic
 * distance measurement system. After system initialization and clock
 * configuration, all required peripherals are set up, including GPIO,
 * UART for serial output, and TIM2 for microsecond timing and input
 * capture.
 *
 * TIM2 runs as a free-running timer with a 1 MHz clock (1 µs resolution).
 * Input capture interrupts are enabled on channel 2 to detect rising and
 * falling edges of the ultrasonic echo signal.
 *
 * The main loop executes a non-blocking state machine that controls the
 * measurement cycle. The state machine triggers the ultrasonic sensor,
 * waits for the echo signal, handles timeout conditions, and outputs the
 * measured distance via UART.
 *
 * Timestamp capture of the echo edges is performed in the timer interrupt
 * callback, while the main loop processes the measurement results and
 * schedules subsequent measurements.
 *
 * @retval int This function does not return.
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
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_2);
  const uint16_t TIMEOUT_LIMIT = 35000;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    switch(state)
    {
      case TRIGGER:
        timeout_start = __HAL_TIM_GET_COUNTER(&htim2);
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim2, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        delay_us(10);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

        state = WAIT_RISING;
        break;
      
      case WAIT_INTERVAL:
        if(HAL_GetTick() - last_time >= 500)
        {
          last_time = HAL_GetTick();
          state = TRIGGER;
        }
        break;

      case WAIT_RISING:
      case WAIT_FALLING:
        if((uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) - timeout_start) > TIMEOUT_LIMIT)
        {
            state = TIMEOUT;
        }
        break;

      case DONE:
        uint16_t diff = (uint16_t)(ic_end - ic_start);

        uint16_t distance_cm = diff / 58;

        HAL_UART_Transmit(&huart2, (uint8_t*)"Distance: ", 10, 100);
        uart_send_uint(distance_cm);
        HAL_UART_Transmit(&huart2, (uint8_t*)" cm\r\n", 5, 100);

        state = WAIT_INTERVAL;
        break;

      case TIMEOUT:
        HAL_UART_Transmit(&huart2, (uint8_t*)"out of range\r\n", 14, 100);
        state = WAIT_INTERVAL;
        break;
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
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
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 LD2_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_0|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

static void delay_us(uint16_t us)
{
  const uint32_t start = __HAL_TIM_GET_COUNTER(&htim2);
  while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim2) - start) < us);
}

static void uart_send_uint(uint16_t value)
{
    char buffer[10];
    int i = 0;

    if (value == 0)
    {
        uint8_t c = '0';
        HAL_UART_Transmit(&huart2, &c, 1, 100);
        return;
    }

    while (value > 0)
    {
        buffer[i] = (value % 10) + '0';
        value /= 10;
        i++;
    }

    while (i != 0)
    {
      i--;
      HAL_UART_Transmit(&huart2, (uint8_t*)&buffer[i], 1, 100);
    }
}



/**
 * @brief Timer input capture interrupt callback for ultrasonic echo measurement.
 *
 * This callback is triggered on input capture events of TIM2 channel 2.
 * It measures the echo pulse width of the ultrasonic sensor using two edges:
 *
 * 1. Rising edge:
 *    - Marks the start of the echo pulse
 *    - Stores the capture value in ic_start
 *    - Switches the capture polarity to falling edge
 *    - Updates the state to US_WAIT_FALLING
 *
 * 2. Falling edge:
 *    - Marks the end of the echo pulse
 *    - Stores the capture value in ic_end
 *    - Restores capture polarity to rising edge
 *    - Updates the state to US_DONE
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    if(state == WAIT_RISING)
    {
      ic_start = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

      state = WAIT_FALLING;

      __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else if(state == WAIT_FALLING)
    {
      ic_end = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);

      state = DONE;

      __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
    }
  }
}

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
