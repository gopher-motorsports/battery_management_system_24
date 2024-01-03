/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "bms.h"
#include "stm32f4xx_hal.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define BMB_UPDATE_PERIOD_MS        50

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

Bms_S gBms;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

// void updatePackTelemetry()
// {
//     static uint32_t lastBmbUpdate = 0;
//     if((HAL_GetTick() - lastBmbUpdate) > BMB_UPDATE_PERIOD_MS)
//     {
//         lastBmbUpdate = HAL_GetTick();
//         updateBmbTelemetry(gBms.bmb, NUM_BMBS_IN_ACCUMULATOR);
//     }
// }

void updateTestData()
{
    static uint32_t lastTestUpdate = 0;
    if((HAL_GetTick() - lastTestUpdate) > 200)
    {
        lastTestUpdate = HAL_GetTick();
        testRead(gBms.bmb, NUM_BMBS_IN_ACCUMULATOR);
    }
}
