#ifndef INC_BMBUPDATETASK_H_
#define INC_BMBUPDATETASK_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "adbms6830.h"
#include <stdbool.h>

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

typedef enum
{
    SADC_REDUNDANT = 0,
    SADC_OW_EVEN,
    SADC_OW_ODD,
    SADC_PAUSE_1,
    SADC_PAUSE_2,
    SADC_PAUSE_3,
    SADC_PAUSE_4,
    SADC_PAUSE_5,
    SADC_PAUSE_6,
    SADC_PAUSE_7,
    NUM_SADC_STATES
} SADC_STATE_E;

/* ==================================================================== */
/* ============================== STRUCTS ============================= */
/* ==================================================================== */

typedef struct
{   
    uint8_t testData[6];
    TRANSACTION_STATUS_E status;

    SENSOR_STATUS_E cellVoltageStatus[NUM_CELLS_PER_BMB];
    float cellVoltage[NUM_CELLS_PER_BMB];

    SENSOR_STATUS_E cellVoltageAvgStatus[NUM_CELLS_PER_BMB];
    float cellVoltageAvg[NUM_CELLS_PER_BMB];

    SENSOR_STATUS_E cellVoltageFilteredStatus[NUM_CELLS_PER_BMB];
    float cellVoltageFiltered[NUM_CELLS_PER_BMB];

    SENSOR_STATUS_E cellVoltageRedundantStatus[NUM_CELLS_PER_BMB];
    float cellVoltageRedundant[NUM_CELLS_PER_BMB];
    bool adcMismatch[NUM_CELLS_PER_BMB];
    bool openWire[NUM_CELLS_PER_BMB+1];

    SENSOR_STATUS_E cellTempStatus[NUM_CELLS_PER_BMB];
    float cellTemp[NUM_CELLS_PER_BMB];

    SENSOR_STATUS_E boardTempStatus;
    float boardTemp;
    
} Bmb_S;

typedef struct
{
	Bmb_S bmb[NUM_BMBS_IN_ACCUMULATOR];
    SADC_STATE_E sadcState;
    float maxCellVoltage;
    float minCellVoltage;
} BmbTaskOutputData_S;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

void initBmbUpdateTask();
void runBmbUpdateTask();

#endif /* INC_BMBUPDATETASK_H_ */
