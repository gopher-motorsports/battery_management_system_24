#ifndef CHARGER_H_
#define CHARGER_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdbool.h>
#include "cellData.h"
#include "packData.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

// Charger TX CAN EXT ID, Charger RX EXT ID is CHARGER_CAN_ID + 1
#define CHARGER_CAN_ID_TX                   0x1806E5F4
// #define CHARGER_CAN_ID_RX                   0x18FF50E5

// The delay between consecutive charger request CAN messages
// The ELCON charger expects a message every 1s and will fault if a message is not recieve in 5s
#define CHARGER_UPDATE_PERIOD_MS			10 

// The timeout for consecutive charger rx data messages 
// The ELCON charger sends a message every 1s and the bms will assume the charger is not connected after this timeout
#define CHARGER_RX_TIMEOUT_MS         5000

// Hysteresis bounds for accumulator imbalance
#define MAX_CELL_IMBALANCE_THRES_HIGH       0.1f
#define MAX_CELL_IMBALANCE_THRES_LOW        0.05f

// Hysteresis bounds for max cell voltage
#define MAX_CELL_VOLTAGE_THRES_HIGH         4.29f
#define MAX_CELL_VOLTAGE_THRES_LOW          4.28f

// Charger output validation thresholds 
// The difference between the charger output and accumulator data must fall below these thresholds
#define CHARGER_VOLTAGE_MISMATCH_THRESHOLD  15.0f
#define CHARGER_CURRENT_MISMATCH_THRESHOLD  5.0f

#define MAH_TO_AH                           1.0f / 1000.0f

#define LOW_CHARGE_VOLTAGE_THRES_V          3.0f

#define MAX_CHARGE_VOLTAGE_V                MAX_BRICK_VOLTAGE * NUM_CELLS_PER_BMB * NUM_BMBS_IN_ACCUMULATOR

#define HIGH_CHARGE_C_RATING                1.0f
#define LOW_CHARGE_C_RATING                 0.1f

#define MAX_CHARGE_CURRENT_A                NUM_PARALLEL_CELLS * MAX_C_RATING * CELL_CAPACITY_MAH * MAH_TO_AH
#define HIGH_CHARGE_CURRENT_A               NUM_PARALLEL_CELLS * HIGH_CHARGE_C_RATING * CELL_CAPACITY_MAH * MAH_TO_AH
#define LOW_CHARGE_CURRENT_A                NUM_PARALLEL_CELLS * LOW_CHARGE_C_RATING * CELL_CAPACITY_MAH * MAH_TO_AH

// The input voltage to the charger in volts
#define CHARGER_INPUT_VOLTAGE_V             120.0f
// The charger's input breaker current limit
#define CHARGER_INPUT_CURRENT_A             20.0f
// The derating factor to apply to the charging current to account for temperature and other environmental factors
#define CHARGER_CURRENT_DERATING_FACTOR     0.85f
// The amount of power that the charger can draw from the wall
#define CHARGER_INPUT_POWER_W               CHARGER_INPUT_VOLTAGE_V * CHARGER_INPUT_CURRENT_A * CHARGER_CURRENT_DERATING_FACTOR
// The expected efficiency of the charger
#define MIN_CHARGER_EFFICIENCY              0.9f
// The amount of power that the charger can output to the battery pack
#define CHARGER_OUTPUT_POWER_W              CHARGER_INPUT_POWER_W * MIN_CHARGER_EFFICIENCY

#define DEFAULT_CURRENT_LIMIT_A             1.0f
#define CELL_VOLTAGE_TAPER_THRESHOLD        4.2f


typedef enum
{
    CHARGER_HARDWARE_FAILURE_ERROR,
    CHARGER_OVERTEMP_ERROR,
    CHARGER_INPUT_VOLTAGE_ERROR,
    CHRAGER_BATTERY_NOT_DETECTED_ERROR,
    CHARGER_COMMUNICATION_ERROR,
    NUM_CHARGER_FAULTS
} Charger_Error_E;

/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */

typedef struct
{
  float chargerVoltage;
	float chargerCurrent;
	bool chargerStatus[NUM_CHARGER_FAULTS];

} Charger_Data_S;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

/*!
  @brief   Send a CAN message to the charger
  @param   voltageRequest - Charger Voltage Request
  @param   currentRequest - Charger Current Request
  @param   enable - Enable/Disable Request
*/
void sendChargerMessage(float voltageRequest, float currentRequest, bool enable);

/*!
  @brief   Update a chargerData struct with charger information
  @param   chargerData - Charger_Data_S data struct to store charger data
*/
// void updateChargerData(Charger_Data_S* chargerData);

#endif /* CHARGER_H_ */
