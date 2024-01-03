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
	HAL_GPIO_WritePin(MAS1_GPIO_Port, MAS1_Pin, SET);
    HAL_GPIO_WritePin(MAS2_GPIO_Port, MAS2_Pin, SET);
}

void runMain()
{
    // updatePackTelemetry();
    updateTestData();

    static uint32_t lastUpdateMain = 0;
    if((HAL_GetTick() - lastUpdateMain) > MAIN_UPDATE_PERIOD_MS)
    {
        lastUpdateMain = HAL_GetTick();
        printf("\e[1;1H\e[2J");
        // printCellVoltages();
        printf("Test Read Result: \n");
        for(int32_t i = 0; i < 6; i++)
        {
            printf("%X\n", gBms.bmb[0].testData[i]);
        }
        
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
            printf("|\n");
        }
    }
	printf("\n");
}