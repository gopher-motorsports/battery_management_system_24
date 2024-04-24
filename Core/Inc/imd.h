#ifndef INC_IMD_H_
#define INC_IMD_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */


/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

// Timeout of IMD PWM signal in milliseconds
#define IMD_PWM_TIMOUT_MS 200


/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
	IMD_NO_SIGNAL = 0,
	IMD_NORMAL,
	IMD_UNDER_VOLT,
	IMD_SPEED_START_MEASUREMENT_GOOD,
	IMD_SPEED_START_MEASUREMENT_BAD,
	IMD_DEVICE_ERROR,
	IMD_EARTH_FAULT
} IMD_State_E;


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

/*!
  @brief   Determine the current status of the IMD from the PWM output
  @returns The current state of the IMD
*/
IMD_State_E getImdStatus();


#endif /* INC_IMD_H_ */
