#ifndef INC_BMBUPDATETASK_H_
#define INC_BMBUPDATETASK_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "adbms6830.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define NUM_BMBS_IN_ACCUMULATOR     1
#define NUM_CELLS_PER_BMB           16

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    UNINITIALIZED = 0,
    GOOD,
    BAD
} SENSOR_STATUS_E;

/* ==================================================================== */
/* ============================== STRUCTS ============================= */
/* ==================================================================== */

typedef struct
{   
    float cellVoltage[NUM_CELLS_PER_BMB];
    uint8_t testData[6];
    TRANSACTION_STATUS_E status;
    SENSOR_STATUS_E cellVoltageStatus[NUM_CELLS_PER_BMB];
} Bmb_S;

typedef struct
{
	Bmb_S bmb[NUM_BMBS_IN_ACCUMULATOR];
} BmbTaskOutputData_S;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

void initBmbUpdateTask();
void runBmbUpdateTask();

#endif /* INC_BMBUPDATETASK_H_ */
