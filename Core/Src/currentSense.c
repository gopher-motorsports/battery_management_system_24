/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "currentSense.h"
#include "GopherCAN.h"
#include <math.h>
#include "debug.h"

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void getTractiveSystemCurrent(CurrentSenseTaskOutputData_S* currentData)
{
    // Fetch current sensor data from gophercan
    float currHI = bmsTractiveSystemCurrentHigh_A.data;
    float currLO = bmsTractiveSystemCurrentLow_A.data;
    

    // If the current exceeds the following threshold in either the positive or negative direction,
    // the sensor input has railed to 0 or 5v and a current sensor error is set
    currentData->currentSensorStatusHI = (fabs(currHI) < CURRENT_HIGH_RAIL_THRESHOLD) ? (GOOD) : (BAD);
    currentData->currentSensorStatusLO = (fabs(currLO) < CURRENT_LOW_RAIL_THRESHOLD) ? (GOOD) : (BAD);

    // To use the HI current sensor channel, it must be working AND (it must exceed the measuring range of the low channel OR the low channel must be faulty)
    if ((currentData->currentSensorStatusHI == GOOD) && ((fabs(currLO) > CURRENT_LOW_TO_HIGH_SWITCH_THRESHOLD + (CHANNEL_FILTERING_WIDTH / 2)) || (currentData->currentSensorStatusLO != GOOD)))
    {
        currentData->tractiveSystemCurrent = currHI;
        currentData->tractiveSystemCurrentStatus = GOOD;
    }
    else if ((currentData->currentSensorStatusHI == GOOD) && (currentData->currentSensorStatusLO == GOOD) && ((fabs(currLO) > CURRENT_LOW_TO_HIGH_SWITCH_THRESHOLD - (CHANNEL_FILTERING_WIDTH / 2))))
    {
        float interpolationStart    =   CURRENT_LOW_TO_HIGH_SWITCH_THRESHOLD  - (CHANNEL_FILTERING_WIDTH / 2);
        float interpolationRatio    =   (currLO - interpolationStart) / CHANNEL_FILTERING_WIDTH;
        float filteredCurrent       =   ((1.0f - interpolationRatio) * currLO) + (interpolationRatio * currHI);
        currentData->tractiveSystemCurrent  =   filteredCurrent;
        currentData->tractiveSystemCurrentStatus = GOOD;
    }
    else if (currentData->currentSensorStatusLO == GOOD) // If the above condition is not satisfied, the LO channel must be working in order to use its data
    {
        currentData->tractiveSystemCurrent = currLO;
        currentData->tractiveSystemCurrentStatus = GOOD;
    }
    else // If both sensors are faulty, no current data can be accurately returned
    {
        currentData->tractiveSystemCurrentStatus = BAD;
        // Debug("Failed to read data from current sensor\n");
    }
}
