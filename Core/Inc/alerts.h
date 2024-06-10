#ifndef INC_ALERTS_H_
#define INC_ALERTS_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "timer.h"
#include "bmbUpdateTask.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define OVERVOLTAGE_WARNING_ALERT_SET_TIME_MS     1000
#define OVERVOLTAGE_WARNING_ALERT_CLEAR_TIME_MS   1000

#define OVERVOLTAGE_FAULT_ALERT_SET_TIME_MS       5000
#define OVERVOLTAGE_FAULT_ALERT_CLEAR_TIME_MS     5000

#define UNDERVOLTAGE_WARNING_ALERT_SET_TIME_MS    2000
#define UNDERVOLTAGE_WARNING_ALERT_CLEAR_TIME_MS  2000

#define UNDERVOLTAGE_FAULT_ALERT_SET_TIME_MS      5000
#define UNDERVOLTAGE_FAULT_ALERT_CLEAR_TIME_MS    5000

#define MAX_CELL_IMBALANCE_V                      0.2f
#define CELL_IMBALANCE_ALERT_SET_TIME_MS          1000
#define CELL_IMBALANCE_ALERT_CLEAR_TIME_MS        1000

#define OVERTEMPERATURE_WARNING_ALERT_SET_TIME_MS   500
#define OVERTEMPERATURE_WARNING_ALERT_CLEAR_TIME_MS 500

#define OVERTEMPERATURE_FAULT_ALERT_SET_TIME_MS     5000
#define OVERTEMPERATURE_FAULT_ALERT_CLEAR_TIME_MS   5000

#define SDC_FAULT_ALERT_SET_TIME_MS   0
#define SDC_FAULT_ALERT_CLEAR_TIME_MS 0

#define CURRENT_SENSOR_ERROR_ALERT_SET_TIME_MS        2000
#define CURRENT_SENSOR_ERROR_ALERT_CLEAR_TIME_MS      2000

#define BMB_COMMUNICATION_FAILURE_ALERT_SET_TIME_MS   5000
#define BMB_COMMUNICATION_FAILURE_ALERT_CLEAR_TIME_MS 5000

#define BAD_VOLTAGE_SENSE_STATUS_ALERT_SET_TIME_MS    2000
#define BAD_VOLTAGE_SENSE_STATUS_ALERT_CLEAR_TIME_MS  2000

#define BAD_BRICK_TEMP_SENSE_STATUS_ALERT_SET_TIME_MS    1000
#define BAD_BRICK_TEMP_SENSE_STATUS_ALERT_CLEAR_TIME_MS  1000

#define BAD_BOARD_TEMP_SENSE_STATUS_ALERT_SET_TIME_MS    1000
#define BAD_BOARD_TEMP_SENSE_STATUS_ALERT_CLEAR_TIME_MS  1000

// The minimum percent of brick temps that must be monitored to pass rules
#define MIN_PERCENT_BRICK_TEMPS_MONITORED             25
#define INSUFFICIENT_TEMP_SENSOR_ALERT_SET_TIME_MS    5000
#define INSUFFICIENT_TEMP_SENSOR_ALERT_CLEAR_TIME_MS  5000

#define STACK_VS_SEGMENT_IMBALANCE_ALERT_SET_TIME_MS    1000
#define STACK_VS_SEGMENT_IMBALANCE_ALERT_CLEAR_TIME_MS  1000

#define CHARGER_OVERVOLTAGE_ALERT_SET_TIME_MS      2000
#define CHARGER_OVERVOLTAGE_ALERT_CLEAR_TIME_MS    2000

#define CHARGER_OVERCURRENT_ALERT_SET_TIME_MS      2000
#define CHARGER_OVERCURRENT_ALERT_CLEAR_TIME_MS    2000

#define CHARGER_VOLTAGE_MISMATCH_ALERT_SET_TIME_MS      2000
#define CHARGER_VOLTAGE_MISMATCH_ALERT_CLEAR_TIME_MS    2000

#define CHARGER_CURRENT_MISMATCH_ALERT_SET_TIME_MS      2000
#define CHARGER_CURRENT_MISMATCH_ALERT_CLEAR_TIME_MS    2000

#define CHARGER_DIAGNOSTIC_ALERT_SET_TIME_MS      2000
#define CHARGER_DIAGNOSTIC_ALERT_CLEAR_TIME_MS    2000

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    ALERT_CLEARED = 0,	// Indicates alert has not been set yet
    ALERT_LATCHED,		// Indicates alert is no longer present, but was at one point (only applies for latching alerts)
    ALERT_SET			// Indicates alert is currently set
} AlertStatus_E;

typedef enum
{
    INFO_ONLY = 0,		// Only used for info no actual response
    DISABLE_BALANCING,	// Disables cell balancing
    EMERGENCY_BLEED,	// Emergencly bleed all the cells down
    DISABLE_CHARGING,	// Disable charging 
    LIMP_MODE,			// Limit max current out of pack
    AMS_FAULT,			// Set AMS fault to open shutdown circuit
    NUM_ALERT_RESPONSES
} AlertResponse_E;

/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */

typedef bool (*bmbAlertCondition)(BmbTaskOutputData_S* bmbData);

typedef struct
{
    const char* alertName;
    // Whether the alert is latching or not
    const bool latching;
    // The current status of the alert
    AlertStatus_E alertStatus;
    // The timer used for qualifying the alert set/clear condition
    Timer_S alertTimer;
    // The time in ms required for the alert to be set
    const uint32_t setTime_MS;
    // The time in ms required for the alert to clear
    const uint32_t clearTime_MS;
    // Function pointer used to determine whether the alert is present or not
    bool alertConditionPresent;
    // The number of alert responses for this alert
    const uint32_t numAlertResponse;
    // Array of alert responses
    const AlertResponse_E* alertResponse;
} Alert_S;



/* ==================================================================== */
/* ======================= EXTERNAL VARIABLES ========================= */
/* ==================================================================== */
// The total number of alerts
extern const uint32_t NUM_BMB_ALERTS; 
// Array of all alert structs
extern Alert_S* bmbAlerts[];

extern bmbAlertCondition bmbAlertConditionArray[];


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

/*!
  @brief   Get the status of any given alert
  @param   alert - The alert data structure whose status to read
  @return  The current status of the alert
*/
AlertStatus_E getAlertStatus(Alert_S* alert);

/*!
  @brief   Run the alert monitor to update the status of the alert
  @param   bms - The BMS data structure
  @param   alert - The Alert data structure
*/
void runAlertMonitor(Alert_S* alert);

void setAmsFault(bool set);

#endif /* INC_ALERTS_H_ */
