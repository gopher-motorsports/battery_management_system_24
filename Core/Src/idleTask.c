#include <stdint.h>
#include "main.h"

#define HEARTBEAT_UPDATE_RATE 500
#define BLINK_DURATION        150
#define BLINK_DELAY           1000


void idleInit()
{
    // HAL_GPIO_WritePin(AMS_FAULT_OUT_GPIO_Port, AMS_FAULT_OUT_Pin, GPIO_PIN_SET);
}

void runIdle()
{
    /* USER CODE BEGIN StartIdle */
  
    // Update Heartbeat LED
    static uint32_t lastHeartbeat = 0;
    if (HAL_GetTick() - lastHeartbeat >= HEARTBEAT_UPDATE_RATE)
    {
      HAL_GPIO_TogglePin(MCU_HEARTBEAT_GPIO_Port, MCU_HEARTBEAT_Pin);
      lastHeartbeat = HAL_GetTick();
    }

    // // Update Fault LED
    // static uint32_t lastBlink = 0;
    // static uint32_t remainingBlinks = 0;

    // switch (gBms.bmsHwState)
    // {
    //   case BMS_NOMINAL:
    //   {
    //     // Ensure fault LED is off
    //     HAL_GPIO_WritePin(MCU_FAULT_GPIO_Port, MCU_FAULT_Pin, GPIO_PIN_RESET);
    //     break;
    //   }
    //   case BMS_GSNS_FAILURE:
    //   {
    //     // Single blink
    //     if (HAL_GetTick() - lastBlink > BLINK_DELAY)
    //     {
    //       // Start a new blink
    //       remainingBlinks = 1;
    //     }
    //     break;
    //   }
    //   case BMS_BMB_FAILURE:
    //   {
    //     // Double blink
    //     if (HAL_GetTick() - lastBlink > BLINK_DELAY)
    //     {
    //       // Start a new double blink
    //       remainingBlinks = 2;
    //     }
    //     break;
    //   }
    // }
    // if (remainingBlinks > 0 && HAL_GetTick() - lastBlink > 2*BLINK_DURATION)
    // {
    //   // Start a new blink sequence
    //   HAL_GPIO_WritePin(MCU_FAULT_GPIO_Port, MCU_FAULT_Pin, GPIO_PIN_SET);
    //   lastBlink = HAL_GetTick();
    //   remainingBlinks--;
    // }
    // if (HAL_GetTick() - lastBlink > BLINK_DURATION)
    // {
    //     HAL_GPIO_WritePin(MCU_FAULT_GPIO_Port, MCU_FAULT_Pin, GPIO_PIN_RESET);
    // }
}
