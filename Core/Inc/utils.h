#ifndef UTILS_H_
#define UTILS_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <stdbool.h>
#include <math.h>
#include "main.h"
#include "debug.h"
#include "cmsis_os.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

// The max spacing between two floats before they are considered not equal
#define EPSILON 1e-4f

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */
    // Interupt status enum for task notification flags

/* ==================================================================== */
/* ============================== MACROS ============================== */
/* ==================================================================== */

/*!
  @brief   Determine if two floating point values are equal
  @returns True if equal, false if not equal
*/
#define fequals(a, b) (fabsf(a - b) < EPSILON) ? (true) : (false)

/*!
  @brief    Blocks the current task and waits for an external interupt to be recieved
  @param    timeout     Max duration of task blocking state
  @param    callingFunc Name of the function that called SPI_TRANSMIT for debugging purposes
*/
#define WAIT_EXT_INT(timeout, callingFunc) \
  ({ \
    /* Notification value used for task notification */ \
    uint32_t notificationFlags = 0; \
    /* Block calling task and wait for external interrupt to occur */ \
    if (xTaskNotifyWait(TASK_NO_OP, TASK_CLEAR_FLAGS, &notificationFlags, timeout) != pdTRUE) \
    { /* External interrupt has failed to occur */ \
      DebugComm("External interrupt failed to occur in %s!\n", #callingFunc); \
    } \
    /* Macro will evaluate to this value - can be used as a return value to handle EXTI failures further */ \
    (INTERRUPT_STATUS_E) notificationFlags; \
  })

#endif /* UTILS_H_ */
