#ifndef INC_ADBMS6830_H_
#define INC_ADBMS6830_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdint.h>

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    TRANSACTION_FAIL = 0 ,
    TRANSACTION_SUCCESS
} TRANSACTION_STATUS_E;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

TRANSACTION_STATUS_E writeAll(uint16_t command, uint8_t *txData, uint32_t numBmbs);

TRANSACTION_STATUS_E readAll(uint16_t command, uint8_t *rxData, uint32_t numBmbs);

#endif /* INC_ADBMS6830_H_ */

