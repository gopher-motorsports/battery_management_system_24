/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdio.h>

#include "printTask.h"
#include "main.h"
#include "bmbUpdateTask.h"
#include "currentSenseTask.h"
#include "cmsis_os.h"
#include "alerts.h"
#include "GopherCAN.h"

/* ==================================================================== */
/* ============================== STRUCTS ============================= */
/* ==================================================================== */

typedef struct
{
    BmbTaskOutputData_S bmbTaskData;
    CurrentSenseTaskOutputData_S currentSenseData;
} PrintTaskInputData_S;

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static void printCellVoltages(Bmb_S* bmb);
static void printCellTemps(Bmb_S* bmb);
static void printTestData(Bmb_S* bmb);
static void printActiveAlerts();
void printSocAndSoe(CurrentSenseTaskOutputData_S currentSenseData);
// static void printTempTest(Bmb_S* bmb);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static void printCellVoltages(Bmb_S* bmb)
{
    printf("Cell Voltage:\n");
    printf("|   CELL   |");
    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        printf("    %02ld     |", i);
    }
    printf("\n");
    // printf("|    00    |");
    // for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    // {
    //     if(bmb[i].openWire[0])
    //     {
    //         printf(" OPEN WIRE |");
    //     }
    //     else
    //     {
    //         printf("           |");
    //     }
    // }
    // printf("\n");

    for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
    {
        printf("|    %02ld    |", i+1);
        for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
        {
            if(bmb[j].openWire[i])
            {
                printf(" OPEN WIRE |");
            }
            else
            {
                if(bmb[j].cellVoltageStatus[i] == GOOD)
                {

                    if(bmb[j].balSwEnabled[i])
                    {
                        printf("*");
                    }
                    else
                    {
                        printf(" ");
                    }

                    if((bmb[j].cellVoltage[i] < 0.0f) || bmb[j].cellVoltage[i] >= 100.0f)
                    {
                        printf(" %5.3f   |", (double)bmb[j].cellVoltage[i]);
                    }
                    else
                    {
                        printf("  %5.3f   |", (double)bmb[j].cellVoltage[i]);
                    }
                }
                else
                {
                    printf(" NO SIGNAL |");
                }
            }
        }
        printf("\n");
    }
	printf("\n");
}

// static void printCellVoltages(Bmb_S* bmb)
// {
//     printf("Cell Voltage:\n");
//     printf("|   CELL  |");
//     printf("    RAW    |    AVG    |    FIL    |    SADC   |    TAB    |");
//     printf("\n");
//     for(int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
//     {
//         printf("|    %02ld   |", i);
//         if(bmb[i].cellVoltageStatus[i] == GOOD)
//         {
//             if((bmb[0].cellVoltage[i] < 0.0f) || bmb[0].cellVoltage[i] >= 100.0f)
//             {
//                 printf("  %5.3f   |", (double)bmb[0].cellVoltage[i]);
//             }
//             else
//             {
//                 printf("   %5.3f   |", (double)bmb[0].cellVoltage[i]);
//             }
//             // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
//         }
//         else
//         {
//             printf(" NO SIGNAL |");
//         }
//         if(bmb[0].cellVoltageAvgStatus[i] == GOOD)
//         {
//             if((bmb[0].cellVoltageAvg[i] < 0.0f) || bmb[0].cellVoltageAvg[i] >= 100.0f)
//             {
//                 printf("  %5.3f   |", (double)bmb[0].cellVoltageAvg[i]);
//             }
//             else
//             {
//                 printf("   %5.3f   |", (double)bmb[0].cellVoltageAvg[i]);
//             }
//             // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
//         }
//         else
//         {
//             printf(" NO SIGNAL |");
//         }
//         if(bmb[0].cellVoltageFilteredStatus[i] == GOOD)
//         {
//             if((bmb[0].cellVoltageFiltered[i] < 0.0f) || bmb[0].cellVoltageFiltered[i] >= 100.0f)
//             {
//                 printf("  %5.3f   |", (double)bmb[0].cellVoltageFiltered[i]);
//             }
//             else
//             {
//                 printf("   %5.3f   |", (double)bmb[0].cellVoltageFiltered[i]);
//             }
//             // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
//         }
//         else
//         {
//             printf(" NO SIGNAL |");
//         }
//         if(bmb[0].cellVoltageRedundantStatus[i] == GOOD)
//         {
//             if((bmb[0].cellVoltageRedundant[i] < 0.0f) || bmb[0].cellVoltageRedundant[i] >= 100.0f)
//             {
//                 printf("  %5.3f   |", (double)bmb[0].cellVoltageRedundant[i]);
//             }
//             else
//             {
//                 printf("   %5.3f   |", (double)bmb[0].cellVoltageRedundant[i]);
//             }
//             // printf("  %04X", gBms.bmb[j].cellVoltage[i]);
//         }
//         else
//         {
//             printf(" NO SIGNAL |");
//         }
//         if(bmb[0].openWire[i])
//         {
//             printf(" OPEN WIRE |");
//         }
//         else
//         {
//             printf("           |");
//         }
//         printf("\n");
//     }
//     printf("|    16   |    ----   |    ----   |    ----   |    ----   |");
//     if(bmb[0].openWire[NUM_CELLS_PER_BMB])
//     {
//         printf(" OPEN WIRE |");
//     }
//     else
//     {
//         printf("           |");
//     }
// 	printf("\n");
//     printf("\n");
// }

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
        printf("|    %02ld   |", i+1);
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
    printf("|   Die   |");
    for(int32_t j = 0; j < NUM_BMBS_IN_ACCUMULATOR; j++)
    {
        if(bmb[j].dieTempStatus == GOOD)
        {
            if((bmb[j].dieTemp < 0.0f) || bmb[j].dieTemp >= 100.0f)
            {
                printf("   %3.1f   |", (double)bmb[j].dieTemp);
            }
            else
            {
                printf("    %3.1f   |", (double)bmb[j].dieTemp);
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

// static void printTestData(Bmb_S* bmb)
// {
//     printf("Test Read Result: \n");
//     for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
//     {
//         printf("%X\n", bmb[0].testData[i]);
//     }
//     printf("STATUS: %lu\n", (uint32_t)bmb[0].status);
//     printf("\n");
// }

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
	for (uint32_t i = 0; i < NUM_BMB_ALERTS; i++)
	{
		Alert_S* alert = bmbAlerts[i];
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

    printf("Alerts Latched:\n");
	uint32_t numLatchedAlerts = 0;
	for (uint32_t i = 0; i < NUM_BMB_ALERTS; i++)
	{
		Alert_S* alert = bmbAlerts[i];
		if (getAlertStatus(alert) == ALERT_LATCHED)
		{
			printf("%s - LATCHED!\n", alert->alertName);
			numLatchedAlerts++;
		}
	}
	if (numLatchedAlerts == 0)
	{
		printf("None\n");
	}
	printf("\n");
}

void printSocAndSoe(CurrentSenseTaskOutputData_S currentSenseData)
{
	printf("| METHOD |    SOC   |    SOE   |\n");
	printf("|  OCV   |  %5.2f%%  |  %5.2f%%  |\n", (double)(100.0f * currentSenseData.soc.socByOcv), (double)(100.0f * currentSenseData.soc.soeByOcv));
	printf("|  CC    |  %5.2f%%  |  %5.2f%%  |\n", (double)(100.0f * currentSenseData.soc.socByCoulombCounting), (double)(100.0f * currentSenseData.soc.soeByCoulombCounting));
	printf("Remaining SOC by OCV qualification time ms: %lu\n", getTimeTilExpirationMs(&currentSenseData.soc.socByOcvGoodTimer));
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
    printTaskInputData.currentSenseData = currentSenseOutputData;
    taskEXIT_CRITICAL();

    printf("\e[1;1H\e[2J");
    printCellVoltages(printTaskInputData.bmbTaskData.bmb);
    printCellTemps(printTaskInputData.bmbTaskData.bmb);

    printf("\n");
    printf("Min Cell V: %f\n", printTaskInputData.bmbTaskData.minCellVoltage);
    printf("Max Cell V: %f\n", printTaskInputData.bmbTaskData.maxCellVoltage);
    printf("Max Cell Temp: %f\n", printTaskInputData.bmbTaskData.maxCellTemp);
    printf("Average Cell Temp: %f\n", printTaskInputData.bmbTaskData.avgCellTemp);
    printf("Tractive Current: %6.3f\n", printTaskInputData.currentSenseData.tractiveSystemCurrent);
    printSocAndSoe(printTaskInputData.currentSenseData);
    printf("\n");

    // printTestData(printTaskInputData.bmbTaskData.bmb);
    printActiveAlerts();

    // printf("\n");
    // printf("Charger Voltage: %f\n", chargerVoltageSetPoint_V.data);
    // printf("Charger Current: %f\n", chargerCurrentSetPoint_A.data);
    // printf("Charger Status: %X\n", chargerStatusByte.data);
    // printf("\n");
}