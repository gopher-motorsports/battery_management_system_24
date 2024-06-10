/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bmbUpdateTask.h"
#include "charger.h"
#include "alerts.h"

#define BMB_UPDATE_TASK_PERIOD_MS   200
#define PRINT_TASK_PERIOD_MS        1000
#define CHARGER_TASK_PERIOD_MS      100

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define AMS_FAULT_OUT_Pin GPIO_PIN_13
#define AMS_FAULT_OUT_GPIO_Port GPIOC
#define AMS_FAULT_SDC_Pin GPIO_PIN_14
#define AMS_FAULT_SDC_GPIO_Port GPIOC
#define BSPD_FAULT_SDC_Pin GPIO_PIN_15
#define BSPD_FAULT_SDC_GPIO_Port GPIOC
#define MCU_FAULT_Pin GPIO_PIN_1
#define MCU_FAULT_GPIO_Port GPIOC
#define MCU_HEARTBEAT_Pin GPIO_PIN_2
#define MCU_HEARTBEAT_GPIO_Port GPIOC
#define CP_EN_Pin GPIO_PIN_3
#define CP_EN_GPIO_Port GPIOC
#define PORTA_CS_Pin GPIO_PIN_4
#define PORTA_CS_GPIO_Port GPIOA
#define BMB_SCK_Pin GPIO_PIN_5
#define BMB_SCK_GPIO_Port GPIOA
#define BMB_MISO_Pin GPIO_PIN_6
#define BMB_MISO_GPIO_Port GPIOA
#define BMB_MOSI_Pin GPIO_PIN_7
#define BMB_MOSI_GPIO_Port GPIOA
#define PORTB_CS_Pin GPIO_PIN_4
#define PORTB_CS_GPIO_Port GPIOC
#define IMD_FAULT_SDC_Pin GPIO_PIN_2
#define IMD_FAULT_SDC_GPIO_Port GPIOD
#define CP_PWM_Pin GPIO_PIN_7
#define CP_PWM_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

extern BmbTaskOutputData_S bmbTaskOutputData;
extern Charger_Data_S chargerTaskOutputData;

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
