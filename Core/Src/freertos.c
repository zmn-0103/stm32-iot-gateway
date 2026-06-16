/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht22.h"
#include "OLED.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 共享传感器数据 (由 SensorTask 写入，其他任务读取) */
DHT22_Data g_sensor_data = {0};
osMutexId_t g_sensor_mutex = NULL;
/* USER CODE END Variables */
/* Definitions for SensorTask */
osThreadId_t SensorTaskHandle;
const osThreadAttr_t SensorTask_attributes = {
  .name = "SensorTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for NetworkTask */
osThreadId_t NetworkTaskHandle;
const osThreadAttr_t NetworkTask_attributes = {
  .name = "NetworkTask",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* USER CODE BEGIN Variables2 */
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart3;
/* USER CODE END Variables2 */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void vSensorTask(void *argument);
void vDisplayTask(void *argument);
void vNetworkTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  osKernelInitialize();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  const osMutexAttr_t sensor_mutex_attr = {
    .name = "SensorMutex"
  };
  g_sensor_mutex = osMutexNew(&sensor_mutex_attr);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of SensorTask */
  SensorTaskHandle = osThreadNew(vSensorTask, NULL, &SensorTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(vDisplayTask, NULL, &DisplayTask_attributes);

  /* creation of NetworkTask */
  NetworkTaskHandle = osThreadNew(vNetworkTask, NULL, &NetworkTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_vSensorTask */
/**
  * @brief  Function implementing the SensorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_vSensorTask */
void vSensorTask(void *argument)
{
  /* USER CODE BEGIN vSensorTask */
  /* 初始化 DWT 延时系统 */
  DHT22_Init();

  DHT22_Data local_data;
  DHT22_Status status;
  char buf[64];

  /* Infinite loop */
  for(;;)
  {
    status = DHT22_Read(&local_data);

    if (status == DHT22_OK) {
      /* 读取成功，更新共享数据 */
      if (osMutexAcquire(g_sensor_mutex, 100) == osOK) {
        g_sensor_data = local_data;
        osMutexRelease(g_sensor_mutex);
      }

      /* 串口输出: 用整数避免 float 格式化问题 */
      int t = (int)(local_data.temperature * 10);
      int h = (int)(local_data.humidity * 10);
      int t_int = t / 10, t_dec = (t < 0 ? -t : t) % 10;
      int h_int = h / 10, h_dec = (h < 0 ? -h : h) % 10;
      snprintf(buf, sizeof(buf), "T:%d.%d H:%d.%d\r\n", t_int, t_dec, h_int, h_dec);
      HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);
    } else {
      snprintf(buf, sizeof(buf), "ERR:%d\r\n", (int)status);
      HAL_UART_Transmit(&huart3, (uint8_t*)buf, strlen(buf), 100);
    }

    /* 采样间隔 2s */
    osDelay(2000);
  }
  /* USER CODE END vSensorTask */
}

/* USER CODE BEGIN Header_vDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vDisplayTask */
void vDisplayTask(void *argument)
{
  /* USER CODE BEGIN vDisplayTask */
  OLED_Init();

  OLED_ShowString(1, 1, "IoT Gateway");
  OLED_ShowString(3, 1, "Temp:");
  OLED_ShowString(4, 1, "Humi:");

  DHT22_Data local;

  for(;;)
  {
    /* 读取共享数据 */
    if (osMutexAcquire(g_sensor_mutex, 100) == osOK) {
      local = g_sensor_data;
      osMutexRelease(g_sensor_mutex);
    }

    /* 更新温度 (Line 3, Column 6 起) */
    OLED_ShowString(3, 6, "         ");
    if (local.valid) {
      int t = (int)(local.temperature * 10);
      char t_buf[8];
      t_buf[0] = (t < 0) ? '-' : ' ';
      int abs_t = (t < 0) ? -t : t;
      t_buf[1] = '0' + abs_t / 100;
      t_buf[2] = '0' + (abs_t / 10) % 10;
      t_buf[3] = '.';
      t_buf[4] = '0' + abs_t % 10;
      t_buf[5] = 'C';
      t_buf[6] = '\0';
      OLED_ShowString(3, 6, t_buf);
    } else {
      OLED_ShowString(3, 6, "--.- C");
    }

    /* 更新湿度 (Line 4, Column 6 起) */
    OLED_ShowString(4, 6, "         ");
    if (local.valid) {
      int h = (int)(local.humidity * 10);
      char h_buf[8];
      int abs_h = (h < 0) ? -h : h;
      h_buf[0] = '0' + abs_h / 100;
      h_buf[1] = '0' + (abs_h / 10) % 10;
      h_buf[2] = '.';
      h_buf[3] = '0' + abs_h % 10;
      h_buf[4] = '%';
      h_buf[5] = '\0';
      OLED_ShowString(4, 6, h_buf);
    } else {
      OLED_ShowString(4, 6, "--.- %");
    }

    osDelay(1000);
  }
  /* USER CODE END vDisplayTask */
}

/* USER CODE BEGIN Header_vNetworkTask */
/**
* @brief Function implementing the NetworkTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vNetworkTask */
void vNetworkTask(void *argument)
{
  /* USER CODE BEGIN vNetworkTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vNetworkTask */
}


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
