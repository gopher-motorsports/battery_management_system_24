/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdio.h>

#include "printTask.h"
#include "main.h"
#include "bmbUpdateTask.h"
#include "cmsis_os.h"
#include "alerts.h"

/* ==================================================================== */
/* ============================== STRUCTS ============================= */
/* ==================================================================== */

typedef struct
{
    BmbTaskOutputData_S bmbTaskData;
} PrintTaskInputData_S;

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static void printCellVoltages(Bmb_S* bmb);
static void printCellTemps(Bmb_S* bmb);
static void printTestData(Bmb_S* bmb);
static void printActiveAlerts();
// static void printTempTest(Bmb_S* bmb);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

// static void printCellVoltages(Bmb_S* bmb)
// {
//     printf("Cell Voltage:\n");
//     printf("|   BMB   |");
//     for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
//     {
//         printf("    %02ld    |", i);
//     }
//     printf("\n");
//     for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
//     {
//         printf("|     %02ld   |", i);
//         for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
//         {
//             if(bmb[j].cellVoltageStatus[i] == GOOD)
//             {
//                 printf("   %5.3f  |", (double)bmb[j].cellVoltage[i]);
//                 // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
//             }
//             else
//             {
//                 printf(" NO SIGNAL |");
//             }
//         }
//         printf("\n");
//     }
// 	printf("\n");
// }

static void printCellVoltages(Bmb_S* bmb)
{
    printf("Cell Voltage:\n");
    printf("|   CELL  |");
    printf("    RAW    |    AVG    |    FIL    |    SADC   |    TAB    |");
    printf("\n");
    for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
    {
        printf("|    %02ld   |", i);
        if(bmb[0].cellVoltageStatus[i] == GOOD)
        {
            printf("   %5.3f  |", (double)bmb[0].cellVoltage[i]);
            // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
        }
        else
        {
            printf(" NO SIGNAL |");
        }
        if(bmb[0].cellVoltageAvgStatus[i] == GOOD)
        {
            printf("   %5.3f  |", (double)bmb[0].cellVoltageAvg[i]);
            // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
        }
        else
        {
            printf(" NO SIGNAL |");
        }
        if(bmb[0].cellVoltageFilteredStatus[i] == GOOD)
        {
            printf("   %5.3f  |", (double)bmb[0].cellVoltageFiltered[i]);
            // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
        }
        else
        {
            printf(" NO SIGNAL |");
        }
        if(bmb[0].cellVoltageRedundantStatus[i] == GOOD)
        {
            printf("   %5.3f  |", (double)bmb[0].cellVoltageRedundant[i]);
            // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
        }
        else
        {
            printf(" NO SIGNAL |");
        }
        if(bmb[0].openWire[i])
        {
            printf(" OPEN WIRE |");
        }
        else
        {
            printf("           |");
        }
        printf("\n");
    }
    printf("|    16   |    ----   |    ----   |    ----   |    ----   |");
    if(bmb[0].openWire[NUM_CELLS_PER_BMB])
    {
        printf(" OPEN WIRE |");
    }
    else
    {
        printf("           |");
    }
	printf("\n");
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

static void printActiveAlerts()
{
	printf("Alerts Active:\n");
	uint32_t numActiveAlerts = 0;
	for (uint32_t i = 0; i < NUM_ALERTS; i++)
	{
		Alert_S* alert = alerts[i];
		if (getAlertStatus(alert) == ALERT_SET)
		{
			printf("%s - ACTIVE!\n", alert->alertName);
			numActiveAlerts++;
		}
	}
	if (numActiveAlerts == 0)
	{
		printf("None\n");
	}
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
    taskEXIT_CRITICAL();

    printf("\e[1;1H\e[2J");
    printCellVoltages(printTaskInputData.bmbTaskData.bmb);
    printCellTemps(printTaskInputData.bmbTaskData.bmb);
    // printTestData(printTaskInputData.bmbTaskData.bmb);
    // printActiveAlerts();
}