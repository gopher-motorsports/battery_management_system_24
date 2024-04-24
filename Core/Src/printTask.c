/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdio.h>
#include <string.h>

#include "printTask.h"
#include "main.h"
#include "bmbUpdateTask.h"
#include "lowPriTask.h"
#include "cmsis_os.h"

#include "imd.h"

/* ==================================================================== */
/* ============================== STRUCTS ============================= */
/* ==================================================================== */

typedef struct
{
    BmbTaskOutputData_S bmbTaskData;
    IMD_State_E imdState;
} PrintTaskInputData_S;

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static void printCellVoltages(Bmb_S* bmb);
static void printCellTemps(Bmb_S* bmb);
static void printTestData(Bmb_S* bmb);
// static void printTempTest(Bmb_S* bmb);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static void printCellVoltages(Bmb_S* bmb)
{
    printf("Cell Voltage:\n");
    printf("|   BMB   |");
    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        printf("    %02ld    |", i);
    }
    printf("\n");
    for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
    {
        printf("|     %02ld   |", i);
        for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
        {
            if(bmb[j].cellVoltageStatus[i] == GOOD)
            {
                printf("   %5.3f  |", (double)bmb[j].cellVoltage[i]);
                // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
            }
            else
            {
                printf(" NO SIGNAL |");
            }
        }
        printf("\n");
    }
	printf("\n");
}

static void printCellTemps(Bmb_S* bmb)
{
    printf("Cell Temp:\n");
    printf("|   BMB   |");
    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        printf("     %02ld    |", i);
    }
    printf("\n");
    for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
    {
        printf("|    %02ld   |", i);
        for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
        {
            if(bmb[j].cellTempStatus[i] == GOOD)
            {
                if((bmb[j].cellTemp[i] < 0.0f) || bmb[j].cellTemp[i] >= 100.0f)
                {
                    printf("   %3.1f   |", (double)bmb[j].cellTemp[i]);
                }
                else
                {
                    printf("    %3.1f   |", (double)bmb[j].cellTemp[i]);
                }
            }
            else
            {
                printf(" NO SIGNAL |");
            }
        }
        printf("\n");
    }
    printf("|  Board  |");
    for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
    {
        if(bmb[j].boardTempStatus == GOOD)
        {
            if((bmb[j].boardTemp < 0.0f) || bmb[j].boardTemp >= 100.0f)
            {
                printf("   %3.1f   |", (double)bmb[j].boardTemp);
            }
            else
            {
                printf("    %3.1f   |", (double)bmb[j].boardTemp);
            }
            // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
        }
        else
        {
            printf(" NO SIGNAL |");
        }
    }
	printf("\n");
}

static void printTestData(Bmb_S* bmb)
{
    printf("Test Read Result: \n");
    for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
    {
        printf("%X\n", bmb[0].testData[i]);
    }
    printf("STATUS: %lu\n", (uint32_t)bmb[0].status);
    printf("\n");
}

// static void printTempTest(Bmb_S* bmb)
// {
//     printf("| %09lu |", HAL_GetTick());
//     for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
//     {
//         if(bmb[0].cellTempStatus[i] == GOOD)
//         {
//             if((bmb[0].cellTemp[i] < 0.0f) || bmb[0].cellTemp[i] >= 100.0f)
//             {
//                 printf("  %3.1f  |", (double)bmb[0].cellTemp[i]);
//             }
//             else
//             {
//                 printf("   %3.1f  |", (double)bmb[0].cellTemp[i]);
//             }
//         }
//         else
//         {
//             printf(" NO SIG  |");
//         }
//     }
//     printf("\n");
// }

const char* IMD_State_To_String(IMD_State_E state) 
{
	switch(state) {
        case IMD_NO_SIGNAL:
            return "IMD_NO_SIGNAL";
        case IMD_NORMAL:
            return "IMD_NORMAL";
        case IMD_UNDER_VOLT:
            return "IMD_UNDER_VOLT";
        case IMD_SPEED_START_MEASUREMENT_GOOD:
            return "IMD_SPEED_START_MEASUREMENT_GOOD";
        case IMD_SPEED_START_MEASUREMENT_BAD:
            return "IMD_SPEED_START_MEASUREMENT_BAD";
        case IMD_DEVICE_ERROR:
            return "IMD_DEVICE_ERROR";
        case IMD_EARTH_FAULT:
            return "IMD_EARTH_FAULT";
        default:
            return "UNKNOWN_STATE";
    }
}

static void printImdStatus(IMD_State_E state)
{
    const char* stateStr = IMD_State_To_String(state);
	printf("IMD State: %s\n", stateStr);
	printf("\n");
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initPrintTask()
{
    
}

void runPrintTask()
{
    PrintTaskInputData_S printTaskInputData;
    taskENTER_CRITICAL();
    printTaskInputData.bmbTaskData = bmbTaskOutputData;
    printTaskInputData.imdState = lowPriTaskOutputData.imdState;
    taskEXIT_CRITICAL();

    printf("\e[1;1H\e[2J");
    printCellVoltages(printTaskInputData.bmbTaskData.bmb);
    printCellTemps(printTaskInputData.bmbTaskData.bmb);
    printTestData(printTaskInputData.bmbTaskData.bmb);
    printImdStatus(printTaskInputData.imdState);
}