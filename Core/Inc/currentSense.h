#ifndef INC_CURRENT_SENSE_H_
#define INC_CURRENT_SENSE_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "currentSenseTask.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

// Define the current sensor in use
#define DHAB_S_133
// #define DHAB_S_118


// Rail thresholds are defined such that the highest and lowest readings of the 
// Current sensor will never reach the rail threshold, but a 0V or 5V signal on the
// analog channel will result in a value that exceeds the threshold

// Low to high thresholds are defined at the highest range of the low channel current sensor

#if defined(DHAB_S_133)

#define CURRENT_HIGH_RAIL_THRESHOLD             852
#define CURRENT_LOW_RAIL_THRESHOLD              87
#define CURRENT_LOW_TO_HIGH_SWITCH_THRESHOLD    75

#elif defined(DHAB_S_118)

#define CURRENT_HIGH_RAIL_THRESHOLD             410
#define CURRENT_LOW_RAIL_THRESHOLD              35
#define CURRENT_LOW_TO_HIGH_SWITCH_THRESHOLD    30

#endif //DHAB_S_133

// The size in Amps across which current from channel 1 is blended with channel 2
#define CHANNEL_FILTERING_WIDTH 4


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

/*!
  @brief   Update the bms tractive system current data and sensor status from gophercan
  @param   bms - BMS data struct
*/
void getTractiveSystemCurrent(CurrentSenseTaskOutputData_S* currentData);


#endif /* INC_CURRENT_SENSE_H_ */
