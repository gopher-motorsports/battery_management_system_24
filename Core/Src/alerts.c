/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "alerts.h"
#include "cellData.h"
#include <math.h>

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static bool overvoltageWarningPresent(BmbTaskOutputData_S* bmbData)
{
    return (bmbData->maxCellVoltage > MAX_BRICK_WARNING_VOLTAGE);
}

static bool overvoltageFaultPresent(BmbTaskOutputData_S* bmbData)
{
    return (bmbData->maxCellVoltage > MAX_BRICK_FAULT_VOLTAGE);
}

static bool undervoltageWarningPresent(BmbTaskOutputData_S* bmbData)
{
    return (bmbData->minCellVoltage < MIN_BRICK_WARNING_VOLTAGE);
}

static bool undervoltageFaultPresent(BmbTaskOutputData_S* bmbData)
{
    return (bmbData->minCellVoltage < MIN_BRICK_FAULT_VOLTAGE);
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

/*!
  @brief   Get the status of any given alert
  @param   alert - The alert data structure whose status to read
  @return  The current status of the alert
*/
AlertStatus_E getAlertStatus(Alert_S* alert)
{
    return alert->alertStatus;
}

/*!
  @brief   Run the alert monitor to update the status of the alert
  @param   bms - The BMS data structure
  @param   alert - The Alert data structure
*/
void runAlertMonitor(Alert_S* alert)
{
    if (alert->alertStatus == ALERT_CLEARED || alert->alertStatus == ALERT_LATCHED)
    {
        // Determine if we need to set the alert
        if (alert->alertConditionPresent)
        {
            // Increment alert timer by 10ms
            updateTimer(&alert->alertTimer);
        }
        else
        {
            // Reset alert timer
            clearTimer(&alert->alertTimer);
        }

        // Determine if alert was set or in the case that the timer threshold is 0 then check whether the alert condition is present
        if (checkTimerExpired(&alert->alertTimer) && (!(alert->alertTimer.timThreshold <= 0) || alert->alertConditionPresent))
        {
            // Timer expired - Set alert
            alert->alertStatus = ALERT_SET;
            // Load timer with alert clear time
            configureTimer(&alert->alertTimer, alert->clearTime_MS);
        }

    }
    else if (alert->alertStatus == ALERT_SET)
    {
        // Determine if we can clear the alert
        if (!alert->alertConditionPresent)
        {
            // Increment clear timer by 10ms
            updateTimer(&alert->alertTimer);
        }
        else
        {
            // Alert conditions detected. Reset clear timer
            clearTimer(&alert->alertTimer);
        }

        // Determine if alert was cleared or in the case that the timer threshold is 0 then check whether the alert condition is not present
        if (checkTimerExpired(&alert->alertTimer) && (!(alert->alertTimer.timThreshold <= 0) || !alert->alertConditionPresent))
        {
            // Timer expired indicating alert is no longer present. Either set status to latched or clear
            if (alert->latching)
            {
                // Latching alerts can't be cleared - set status to latched to indicate that conditions are no longer met
                alert->alertStatus = ALERT_LATCHED;
            }
            else
            {
                // If non latching alert, the alert can be cleared
                alert->alertStatus = ALERT_CLEARED;
            }
            // Load timer with alert set time
            configureTimer(&alert->alertTimer, alert->setTime_MS);
        }
    }
}

/* ==================================================================== */
/* ========================= GLOBAL VARIABLES ========================= */
/* ==================================================================== */

// Overvoltage Warning Alert
const AlertResponse_E overvoltageWarningAlertResponse[] = { DISABLE_CHARGING };
#define NUM_OVERVOLTAGE_WARNING_ALERT_RESPONSE sizeof(overvoltageWarningAlertResponse) / sizeof(AlertResponse_E)
Alert_S overvoltageWarningAlert =
{ 
    .alertName = "OvervoltageWarning",
    .alertStatus = ALERT_CLEARED, .alertTimer = (Timer_S){.timCount = 0, .lastUpdate = 0, .timThreshold = OVERVOLTAGE_WARNING_ALERT_SET_TIME_MS}, 
    .setTime_MS = OVERVOLTAGE_WARNING_ALERT_SET_TIME_MS, .clearTime_MS = OVERVOLTAGE_WARNING_ALERT_CLEAR_TIME_MS, 
    .alertConditionPresent = false, 
    .numAlertResponse = NUM_OVERVOLTAGE_WARNING_ALERT_RESPONSE, .alertResponse =  overvoltageWarningAlertResponse
};

// Undervoltage Warning Alert
const AlertResponse_E undervoltageWarningAlertResponse[] = { LIMP_MODE };
#define NUM_UNDERVOLTAGE_WARNING_ALERT_RESPONSE sizeof(undervoltageWarningAlertResponse) / sizeof(AlertResponse_E)
Alert_S undervoltageWarningAlert = 
{ 
    .alertName = "UndervoltageWarning",
    .alertStatus = ALERT_CLEARED, .alertTimer = (Timer_S){.timCount = 0, .lastUpdate = 0, .timThreshold = UNDERVOLTAGE_WARNING_ALERT_SET_TIME_MS}, 
    .setTime_MS = UNDERVOLTAGE_WARNING_ALERT_SET_TIME_MS, .clearTime_MS = UNDERVOLTAGE_WARNING_ALERT_CLEAR_TIME_MS, 
    .alertConditionPresent = false,
    .numAlertResponse = NUM_UNDERVOLTAGE_WARNING_ALERT_RESPONSE, .alertResponse = undervoltageWarningAlertResponse
};

// Overvoltage Fault Alert
const AlertResponse_E overvoltageFaultAlertResponse[] = { DISABLE_CHARGING, EMERGENCY_BLEED, AMS_FAULT};
#define NUM_OVERVOLTAGE_FAULT_ALERT_RESPONSE sizeof(overvoltageFaultAlertResponse) / sizeof(AlertResponse_E)
Alert_S overvoltageFaultAlert = 
{ 
    .alertName = "OvervoltageFault", .latching = true,
    .alertStatus = ALERT_CLEARED, .alertTimer = (Timer_S){.timCount = 0, .lastUpdate = 0, .timThreshold = OVERVOLTAGE_FAULT_ALERT_SET_TIME_MS}, 
    .setTime_MS = OVERVOLTAGE_FAULT_ALERT_SET_TIME_MS, .clearTime_MS = OVERVOLTAGE_FAULT_ALERT_CLEAR_TIME_MS, 
    .alertConditionPresent = false, 
    .numAlertResponse = NUM_OVERVOLTAGE_FAULT_ALERT_RESPONSE, .alertResponse =  overvoltageFaultAlertResponse
};

// Undervoltage Fault Alert
const AlertResponse_E undervoltageFaultAlertResponse[] = { AMS_FAULT };
#define NUM_UNDERVOLTAGE_FAULT_ALERT_RESPONSE sizeof(undervoltageFaultAlertResponse) / sizeof(AlertResponse_E)
Alert_S undervoltageFaultAlert = 
{ 
    .alertName = "UndervoltageFault", .latching = true,
    .alertStatus = ALERT_CLEARED, .alertTimer = (Timer_S){.timCount = 0, .lastUpdate = 0, .timThreshold = UNDERVOLTAGE_FAULT_ALERT_SET_TIME_MS}, 
    .setTime_MS = UNDERVOLTAGE_FAULT_ALERT_SET_TIME_MS, .clearTime_MS = UNDERVOLTAGE_FAULT_ALERT_CLEAR_TIME_MS, 
    .alertConditionPresent = false,
    .numAlertResponse = NUM_UNDERVOLTAGE_FAULT_ALERT_RESPONSE, .alertResponse = undervoltageFaultAlertResponse
};

Alert_S* alerts[] = 
{
    &overvoltageWarningAlert,
    &undervoltageWarningAlert,
    &overvoltageFaultAlert,
    &undervoltageFaultAlert
};

// Number of alerts
const uint32_t NUM_ALERTS = sizeof(alerts) / sizeof(Alert_S*);
