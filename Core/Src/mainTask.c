/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "mainTask.h"
#include "main.h"
#include "bms.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define MAIN_UPDATE_PERIOD_MS 1000

/* ==================================================================== */
/* ======================= EXTERNAL VARIABLES ========================= */
/* ==================================================================== */

extern Bms_S 	gBms;

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static void printCellVoltages();

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initMain()
{
	
}

void runMain()
{
    updatePackTelemetry();

    static uint32_t lastUpdateMain = 0;
    if((HAL_GetTick() - lastUpdateMain) > MAIN_UPDATE_PERIOD_MS)
    {
        lastUpdateMain = HAL_GetTick();
        printCellVoltages();
    }
}

static void printCellVoltages()
{
    printf("Cell Voltage:\n");
    printf("|   BMB   |");
    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        printf("    %02ld   |", i);
    }
    printf("\n");
    for(int32_t i = 0; i < NUM_BRICKS_PER_BMB; i++)
    {
        printf("|    %02ld   |", i);
        for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
        {
            if(gBms.bmb[j].cellVoltageStatus[i] == GOOD)
            {
                printf("  %5.3f", (double)gBms.bmb[j].cellVoltage[i]);
            }
            else
            {
                printf("NO SIGNAL");
            }
        }
    }
	printf("\n");
}