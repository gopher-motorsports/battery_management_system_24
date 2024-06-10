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

#define NUM_BMBS_IN_ACCUMULATOR     2
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
    SW_BALANCE,
    NUM_S_PIN_STATES
} S_PIN_STATE_E;

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
    uint16_t openAdcMask;
    bool openWire[NUM_CELLS_PER_BMB+1];

    SENSOR_STATUS_E cellTempStatus[NUM_CELLS_PER_BMB];
    float cellTemp[NUM_CELLS_PER_BMB];

    SENSOR_STATUS_E boardTempStatus;
    float boardTemp;

    SENSOR_STATUS_E dieTempStatus;
    float dieTemp;

    float maxCellVoltage;
    float minCellVoltage;
    float sumCellVoltage;
    float avgCellVoltage;
    uint32_t numBadCellV;

    float maxCellTemp;
    float minCellTemp;
    float avgCellTemp;
    uint32_t numBadCellTemp;

    // Balancing Configuration
	bool balSwRequested[NUM_CELLS_PER_BMB];	// Set by BMS to determine which cells need to be balanced
	bool balSwEnabled[NUM_CELLS_PER_BMB];
    
} Bmb_S;

typedef struct
{
	Bmb_S bmb[NUM_BMBS_IN_ACCUMULATOR];
    S_PIN_STATE_E sPinState;
    // float maxCellVoltage;
    // float minCellVoltage;
    // uint8_t minCellLocationBmb;
    // uint8_t minCellLocation;

    float accumulatorVoltage;

	float maxCellVoltage;
	float minCellVoltage;
	float avgBrickV;

	float maxBrickTemp;
	float minBrickTemp;
	float avgBrickTemp;

	float maxBoardTemp;
	float minBoardTemp;
	float avgBoardTemp;
		// Set by BMB based on ability to balance in hardware
} BmbTaskOutputData_S;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

void initBmbUpdateTask();
void runBmbUpdateTask();

#endif /* INC_BMBUPDATETASK_H_ */
