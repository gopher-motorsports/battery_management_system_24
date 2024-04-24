/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdint.h>
#include "main.h"
#include "imd.h"


/* ==================================================================== */
/* ======================= EXTERNAL VARIABLES ========================= */
/* ==================================================================== */

extern volatile uint32_t imdFrequency;
extern volatile uint32_t imdDutyCycle;
extern volatile uint32_t imdLastUpdate;


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */
/*!
  @brief   Determine the current status of the IMD from the PWM output
  @returns The current state of the IMD
*/
IMD_State_E getImdStatus()
{
    printf("IMD FREQ: %lu\n", imdFrequency);
    printf("IMD DUTY: %lu\n", imdDutyCycle);
    printf("IMD LAST: %lu\n", imdLastUpdate);

	if (imdFrequency > 45)
	{
		return IMD_EARTH_FAULT;
	}
	else if (imdFrequency > 35)
	{
		return IMD_DEVICE_ERROR;
	}
	else if (imdFrequency > 25)
	{
		if(imdDutyCycle < 15)
		{
			return IMD_SPEED_START_MEASUREMENT_GOOD;
		}
		else
		{
			return IMD_SPEED_START_MEASUREMENT_BAD;
		}
	}
	else if (imdFrequency > 15)
	{
		return IMD_UNDER_VOLT;
	}
	else if (imdFrequency > 5)
	{
		return IMD_NORMAL;
	}
	else
	{
		return IMD_NO_SIGNAL;
	}

	if((HAL_GetTick() - imdLastUpdate) > IMD_PWM_TIMOUT_MS)
	{
		return IMD_NO_SIGNAL;
	}
}
