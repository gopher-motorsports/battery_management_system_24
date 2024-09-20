#ifndef SPI_H_
#define SPI_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "cmsis_os.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

// Task notification flags
#define TASK_NO_OP          0UL
#define TASK_CLEAR_FLAGS    0xffffffffUL

// Number of SPI retry events
#define NUM_SPI_RETRY       3

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum 
{
    SPI_TIMEOUT = 0,  // SPI timed out
    SPI_SUCCESS,      // SPI was successful
    SPI_ERROR         // SPI error occured
} SPI_STATUS_E;       // Interupt status enum for task notification flags

/* ==================================================================== */
/* ============================== MACROS ============================== */
/* ==================================================================== */

/*!
  @brief    Carries out a HAL SPI command and blocks the executing task until SPI complete
  @param    fn          HAL SPI function to execute
  @param    hspi        SPI bus handle for transaction
  @param    timeout     Max duration of task blocking state
  @return   Returns a SPI_STATUS_E corresponding to the resulting state of the transaction
*/
#define SPI_TRANSMIT(fn, hspi, timeout, ...) \
  ({ \
    /* Notification value used for task notification */ \
    uint32_t notificationFlags = 0; \
    /* SPI transaction will not be attempted more than NUM_SPI_RETRY */ \
    for (uint32_t attemptNum = 0; attemptNum < NUM_SPI_RETRY; attemptNum++) \
    { \
      /* Attempt to start SPI transaction */ \
      if (fn(hspi, __VA_ARGS__) != HAL_OK) \
      { \
        /* If SPI fails to start, HAL must abort transaction. SPI retries */ \
        HAL_SPI_Abort_IT(hspi); \
        notificationFlags = SPI_ERROR; \
        continue; \
      } \
      /* Wait for SPI interrupt to occur. NotificationFlags will hold notification value indicating status of transaction */ \
      if (xTaskNotifyWait(TASK_NO_OP, TASK_CLEAR_FLAGS, &notificationFlags, timeout) != pdTRUE) \
      { \
        /* If no SPI interrupt occurs in time, transaction is aborted to prevent any longer delay */\
        HAL_SPI_Abort_IT(hspi); \
        notificationFlags = SPI_TIMEOUT; \
        break; \
      } \
      /* If SPI SUCCESS bit is not set in notification value, SPI error has occured. SPI retries */ \
      if (!(notificationFlags & SPI_SUCCESS)) \
      { \
        continue; \
      } \
      /* Break on successful transaction */ \
      break; \
    } \
    /* Macro will evaluate to this value - can be used as a return value to handle SPI failures further */ \
    (SPI_STATUS_E) notificationFlags; \
  })

#endif /* SPI_H_ */
