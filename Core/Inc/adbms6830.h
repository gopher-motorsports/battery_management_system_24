#ifndef INC_ADBMS6830_H_
#define INC_ADBMS6830_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdint.h>

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define BITS_IN_BYTE        8
#define BITS_IN_WORD        16
#define BYTES_IN_WORD       2

#define COMMAND_SIZE_BYTES       2
#define REGISTER_SIZE_BYTES      6

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    TRANSACTION_CRC_ERROR = 0,
    TRANSACTION_SPI_ERROR,
    TRANSACTION_POR_ERROR,
    TRANSACTION_COMMAND_COUNTER_ERROR,
    TRANSACTION_WRITE_REJECT,
    TRANSACTION_SUCCESS
} TRANSACTION_STATUS_E;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void wakeChain(uint32_t numBmbs);
TRANSACTION_STATUS_E commandAll(uint16_t command, uint32_t numBmbs);
TRANSACTION_STATUS_E writeAll(uint16_t command, uint32_t numBmbs, uint8_t *txData);
TRANSACTION_STATUS_E readAll(uint16_t command, uint32_t numBmbs, uint8_t *rxData);

#endif /* INC_ADBMS6830_H_ */

