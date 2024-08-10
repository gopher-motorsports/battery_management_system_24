/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "gcanUpdateTask.h"
#include "GopherCAN.h"
#include "gopher_sense.h"
#include "main.h"

// The number of gsense alerts that need to be logged
#define NUM_GSENSE_ALERTS					26
// #define NUM_BOARD_TEMP_PER_BMB              16

#define GOPHER_CAN_LOGGING_PERIOD_MS      1000

extern CAN_HandleTypeDef hcan2;

/* ==================================================================== */
/* ============================== STRUCTS ============================= */
/* ==================================================================== */

typedef struct
{
    BmbTaskOutputData_S bmbTaskData;
    CurrentSenseTaskOutputData_S currentSenseData;
} GcanTaskInputData;


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initGcanUpdateTask()
{
    
}

void runGcanUpdateTask()
{
    GcanTaskInputData gcanTaskInputData;
    taskENTER_CRITICAL();
	gcanTaskInputData.currentSenseData.soc.soeByOcv = currentSenseOutputData.soc.soeByOcv;
	gcanTaskInputData.currentSenseData.soc.soeByCoulombCounting = currentSenseOutputData.soc.soeByCoulombCounting;

	gcanTaskInputData.bmbTaskData.avgBrickV = bmbTaskOutputData.avgBrickV;
	gcanTaskInputData.bmbTaskData.maxCellVoltage = bmbTaskOutputData.maxCellVoltage;
	gcanTaskInputData.bmbTaskData.minCellVoltage = bmbTaskOutputData.minCellVoltage;

	gcanTaskInputData.bmbTaskData.avgCellTemp = bmbTaskOutputData.avgCellTemp;
	gcanTaskInputData.bmbTaskData.maxCellTemp = bmbTaskOutputData.maxCellTemp;
	gcanTaskInputData.bmbTaskData.minCellTemp = bmbTaskOutputData.minCellTemp;

	gcanTaskInputData.bmbTaskData.avgBoardTemp = bmbTaskOutputData.avgBoardTemp;
	gcanTaskInputData.bmbTaskData.maxBoardTemp = bmbTaskOutputData.maxBoardTemp;
	gcanTaskInputData.bmbTaskData.minBoardTemp = bmbTaskOutputData.minBoardTemp;

    // gcanTaskInputData.bmbTaskData = bmbTaskOutputData;
    // gcanTaskInputData.currentSenseData = currentSenseOutputData;
    taskEXIT_CRITICAL();

    // This condition is to stop gcan sending if we are testing with less than the full accumulator
	// Otherwise, it will try to reference bmb indices that dont exist
	if((NUM_BMBS_IN_ACCUMULATOR == 8) && (NUM_CELLS_PER_BMB == 16))
	{
		// static FLOAT_CAN_STRUCT *cellVoltageParams[NUM_BMBS_IN_ACCUMULATOR][NUM_CELLS_PER_BMB] =
		// {
		// 	{&seg1Cell1Voltage_V, &seg1Cell2Voltage_V, &seg1Cell3Voltage_V, &seg1Cell4Voltage_V, &seg1Cell5Voltage_V, &seg1Cell6Voltage_V, &seg1Cell7Voltage_V, &seg1Cell8Voltage_V, &seg1Cell9Voltage_V, &seg1Cell10Voltage_V, &seg1Cell11Voltage_V, &seg1Cell12Voltage_V},
		// 	{&seg2Cell1Voltage_V, &seg2Cell2Voltage_V, &seg2Cell3Voltage_V, &seg2Cell4Voltage_V, &seg2Cell5Voltage_V, &seg2Cell6Voltage_V, &seg2Cell7Voltage_V, &seg2Cell8Voltage_V, &seg2Cell9Voltage_V, &seg2Cell10Voltage_V, &seg2Cell11Voltage_V, &seg2Cell12Voltage_V},
		// 	{&seg3Cell1Voltage_V, &seg3Cell2Voltage_V, &seg3Cell3Voltage_V, &seg3Cell4Voltage_V, &seg3Cell5Voltage_V, &seg3Cell6Voltage_V, &seg3Cell7Voltage_V, &seg3Cell8Voltage_V, &seg3Cell9Voltage_V, &seg3Cell10Voltage_V, &seg3Cell11Voltage_V, &seg3Cell12Voltage_V},
		// 	{&seg4Cell1Voltage_V, &seg4Cell2Voltage_V, &seg4Cell3Voltage_V, &seg4Cell4Voltage_V, &seg4Cell5Voltage_V, &seg4Cell6Voltage_V, &seg4Cell7Voltage_V, &seg4Cell8Voltage_V, &seg4Cell9Voltage_V, &seg4Cell10Voltage_V, &seg4Cell11Voltage_V, &seg4Cell12Voltage_V},
		// 	{&seg5Cell1Voltage_V, &seg5Cell2Voltage_V, &seg5Cell3Voltage_V, &seg5Cell4Voltage_V, &seg5Cell5Voltage_V, &seg5Cell6Voltage_V, &seg5Cell7Voltage_V, &seg5Cell8Voltage_V, &seg5Cell9Voltage_V, &seg5Cell10Voltage_V, &seg5Cell11Voltage_V, &seg5Cell12Voltage_V},
		// 	{&seg6Cell1Voltage_V, &seg6Cell2Voltage_V, &seg6Cell3Voltage_V, &seg6Cell4Voltage_V, &seg6Cell5Voltage_V, &seg6Cell6Voltage_V, &seg6Cell7Voltage_V, &seg6Cell8Voltage_V, &seg6Cell9Voltage_V, &seg6Cell10Voltage_V, &seg6Cell11Voltage_V, &seg6Cell12Voltage_V},
		// 	{&seg7Cell1Voltage_V, &seg7Cell2Voltage_V, &seg7Cell3Voltage_V, &seg7Cell4Voltage_V, &seg7Cell5Voltage_V, &seg7Cell6Voltage_V, &seg7Cell7Voltage_V, &seg7Cell8Voltage_V, &seg7Cell9Voltage_V, &seg7Cell10Voltage_V, &seg7Cell11Voltage_V, &seg7Cell12Voltage_V}
		// };

		// static FLOAT_CAN_STRUCT *cellTempParams[NUM_BMBS_IN_ACCUMULATOR][NUM_CELLS_PER_BMB] =
		// {
		// 	{&seg1Cell1Temp_C, &seg1Cell2Temp_C, &seg1Cell3Temp_C, &seg1Cell4Temp_C, &seg1Cell5Temp_C, &seg1Cell6Temp_C, &seg1Cell7Temp_C, &seg1Cell8Temp_C, &seg1Cell9Temp_C, &seg1Cell10Temp_C, &seg1Cell11Temp_C, &seg1Cell12Temp_C},
		// 	{&seg2Cell1Temp_C, &seg2Cell2Temp_C, &seg2Cell3Temp_C, &seg2Cell4Temp_C, &seg2Cell5Temp_C, &seg2Cell6Temp_C, &seg2Cell7Temp_C, &seg2Cell8Temp_C, &seg2Cell9Temp_C, &seg2Cell10Temp_C, &seg2Cell11Temp_C, &seg2Cell12Temp_C},
		// 	{&seg3Cell1Temp_C, &seg3Cell2Temp_C, &seg3Cell3Temp_C, &seg3Cell4Temp_C, &seg3Cell5Temp_C, &seg3Cell6Temp_C, &seg3Cell7Temp_C, &seg3Cell8Temp_C, &seg3Cell9Temp_C, &seg3Cell10Temp_C, &seg3Cell11Temp_C, &seg3Cell12Temp_C},
		// 	{&seg4Cell1Temp_C, &seg4Cell2Temp_C, &seg4Cell3Temp_C, &seg4Cell4Temp_C, &seg4Cell5Temp_C, &seg4Cell6Temp_C, &seg4Cell7Temp_C, &seg4Cell8Temp_C, &seg4Cell9Temp_C, &seg4Cell10Temp_C, &seg4Cell11Temp_C, &seg4Cell12Temp_C},
		// 	{&seg5Cell1Temp_C, &seg5Cell2Temp_C, &seg5Cell3Temp_C, &seg5Cell4Temp_C, &seg5Cell5Temp_C, &seg5Cell6Temp_C, &seg5Cell7Temp_C, &seg5Cell8Temp_C, &seg5Cell9Temp_C, &seg5Cell10Temp_C, &seg5Cell11Temp_C, &seg5Cell12Temp_C},
		// 	{&seg6Cell1Temp_C, &seg6Cell2Temp_C, &seg6Cell3Temp_C, &seg6Cell4Temp_C, &seg6Cell5Temp_C, &seg6Cell6Temp_C, &seg6Cell7Temp_C, &seg6Cell8Temp_C, &seg6Cell9Temp_C, &seg6Cell10Temp_C, &seg6Cell11Temp_C, &seg6Cell12Temp_C},
		// 	{&seg7Cell1Temp_C, &seg7Cell2Temp_C, &seg7Cell3Temp_C, &seg7Cell4Temp_C, &seg7Cell5Temp_C, &seg7Cell6Temp_C, &seg7Cell7Temp_C, &seg7Cell8Temp_C, &seg7Cell9Temp_C, &seg7Cell10Temp_C, &seg7Cell11Temp_C, &seg7Cell12Temp_C}
		// };

		// static FLOAT_CAN_STRUCT *boardTempParams[NUM_BMBS_IN_ACCUMULATOR][NUM_CELLS_PER_BMB] =
		// {
		// 	{&seg1BMBBoardTemp1_C, &seg1BMBBoardTemp2_C, &seg1BMBBoardTemp3_C, &seg1BMBBoardTemp4_C},
		// 	{&seg2BMBBoardTemp1_C, &seg2BMBBoardTemp2_C, &seg2BMBBoardTemp3_C, &seg2BMBBoardTemp4_C},
		// 	{&seg3BMBBoardTemp1_C, &seg3BMBBoardTemp2_C, &seg3BMBBoardTemp3_C, &seg3BMBBoardTemp4_C},
		// 	{&seg4BMBBoardTemp1_C, &seg4BMBBoardTemp2_C, &seg4BMBBoardTemp3_C, &seg4BMBBoardTemp4_C},
		// 	{&seg5BMBBoardTemp1_C, &seg5BMBBoardTemp2_C, &seg5BMBBoardTemp3_C, &seg5BMBBoardTemp4_C},
		// 	{&seg6BMBBoardTemp1_C, &seg6BMBBoardTemp2_C, &seg6BMBBoardTemp3_C, &seg6BMBBoardTemp4_C},
		// 	{&seg7BMBBoardTemp1_C, &seg7BMBBoardTemp2_C, &seg7BMBBoardTemp3_C, &seg7BMBBoardTemp4_C}
		// };

		// static FLOAT_CAN_STRUCT *cellVoltageStatsParams[NUM_BMBS_IN_ACCUMULATOR][4] =
		// {
		// 	{&seg1Voltage_V, &seg1AveCellVoltage_V, &seg1MaxCellVoltage_V, &seg1MinCellVoltage_V},
		// 	{&seg2Voltage_V, &seg2AveCellVoltage_V, &seg2MaxCellVoltage_V, &seg2MinCellVoltage_V},
		// 	{&seg3Voltage_V, &seg3AveCellVoltage_V, &seg3MaxCellVoltage_V, &seg3MinCellVoltage_V},
		// 	{&seg4Voltage_V, &seg4AveCellVoltage_V, &seg4MaxCellVoltage_V, &seg4MinCellVoltage_V},
		// 	{&seg5Voltage_V, &seg5AveCellVoltage_V, &seg5MaxCellVoltage_V, &seg5MinCellVoltage_V},
		// 	{&seg6Voltage_V, &seg6AveCellVoltage_V, &seg6MaxCellVoltage_V, &seg6MinCellVoltage_V},
		// 	{&seg7Voltage_V, &seg7AveCellVoltage_V, &seg7MaxCellVoltage_V, &seg7MinCellVoltage_V}
		// };

		// static FLOAT_CAN_STRUCT *cellTempStatsParams[NUM_BMBS_IN_ACCUMULATOR][3] =
		// {
		// 	{&seg1AveCellTemp_C, &seg1MaxCellTemp_C, &seg1MinCellTemp_C},
		// 	{&seg2AveCellTemp_C, &seg2MaxCellTemp_C, &seg2MinCellTemp_C},
		// 	{&seg3AveCellTemp_C, &seg3MaxCellTemp_C, &seg3MinCellTemp_C},
		// 	{&seg4AveCellTemp_C, &seg4MaxCellTemp_C, &seg4MinCellTemp_C},
		// 	{&seg5AveCellTemp_C, &seg5MaxCellTemp_C, &seg5MinCellTemp_C},
		// 	{&seg6AveCellTemp_C, &seg6MaxCellTemp_C, &seg6MinCellTemp_C},
		// 	{&seg7AveCellTemp_C, &seg7MaxCellTemp_C, &seg7MinCellTemp_C}
		// };

		// static FLOAT_CAN_STRUCT *boardTempStatsParams[NUM_BMBS_IN_ACCUMULATOR][3] =
		// {
		// 	{&seg1BMBAveBoardTemp_C, &seg1BMBMaxBoardTemp_C, &seg1BMBMinBoardTemp_C},
		// 	{&seg2BMBAveBoardTemp_C, &seg2BMBMaxBoardTemp_C, &seg2BMBMinBoardTemp_C},
		// 	{&seg3BMBAveBoardTemp_C, &seg3BMBMaxBoardTemp_C, &seg3BMBMinBoardTemp_C},
		// 	{&seg4BMBAveBoardTemp_C, &seg4BMBMaxBoardTemp_C, &seg4BMBMinBoardTemp_C},
		// 	{&seg5BMBAveBoardTemp_C, &seg5BMBMaxBoardTemp_C, &seg5BMBMinBoardTemp_C},
		// 	{&seg6BMBAveBoardTemp_C, &seg6BMBMaxBoardTemp_C, &seg6BMBMinBoardTemp_C},
		// 	{&seg7BMBAveBoardTemp_C, &seg7BMBMaxBoardTemp_C, &seg7BMBMinBoardTemp_C}
		// };

		// static U8_CAN_STRUCT *alertParams[NUM_GSENSE_ALERTS] =
		// {
		// 	&bmsOvervoltageWarningAlert_state,
		// 	&bmsUndervoltageWarningAlert_state,
		// 	&bmsOvervoltageFaultAlert_state,
		// 	&bmsUndervoltageFaultAlert_state,
		// 	&bmsCellImbalanceAlert_state,
		// 	&bmsOvertempWarningAlert_state,
		// 	&bmsOvertempFaultAlert_state,
		// 	&bmsAmsSdcFaultAlert_state,
		// 	&bmsBspdSdcFaultAlert_state,
		// 	&bmsImdSdcFaultAlert_state,
		// 	&bmsCurrentSensorErrorAlert_state,
		// 	&bmsBmbCommunicationFailureAlert_state,
		// 	&bmsBadVoltageSenseStatusAlert_state,
		// 	&bmsBadBrickTempSenseStatusAlert_state,
		// 	&bmsBadBoardTempSenseStatusAlert_state,
		// 	&bmsInsufficientTempSensorsAlert_state,
		// 	&bmsStackVsSegmentImbalanceAlert_state,
		// 	&bmsChargerOverVoltageAlert_state,
		// 	&bmsChargerOverCurrentAlert_state,
		// 	&bmsChargerVoltageMismatchAlert_state,
		// 	&bmsChargerCurrentMismatchAlert_state,
		// 	&bmsChargerHardwareFailureAlert_state,
		// 	&bmsChargerOverTempAlert_state,
		// 	&bmsChargerInputVoltageErrorAlert_state,
		// 	&bmsChargerBatteryNotDetectedErrorAlert_state,
		// 	&bmsChargerCommunicationErrorAlert_state
		// };

		// static U8_CAN_STRUCT *cellVoltageStatusParams[NUM_BMBS_IN_ACCUMULATOR][NUM_CELLS_PER_BMB] =
		// {
		// 	{&seg1Cell1VoltageStatus, &seg1Cell2VoltageStatus, &seg1Cell3VoltageStatus, &seg1Cell4VoltageStatus, &seg1Cell5VoltageStatus, &seg1Cell6VoltageStatus, &seg1Cell7VoltageStatus, &seg1Cell8VoltageStatus, &seg1Cell9VoltageStatus, &seg1Cell10VoltageStatus, &seg1Cell11VoltageStatus, &seg1Cell12VoltageStatus},
		// 	{&seg2Cell1VoltageStatus, &seg2Cell2VoltageStatus, &seg2Cell3VoltageStatus, &seg2Cell4VoltageStatus, &seg2Cell5VoltageStatus, &seg2Cell6VoltageStatus, &seg2Cell7VoltageStatus, &seg2Cell8VoltageStatus, &seg2Cell9VoltageStatus, &seg2Cell10VoltageStatus, &seg2Cell11VoltageStatus, &seg2Cell12VoltageStatus},
		// 	{&seg3Cell1VoltageStatus, &seg3Cell2VoltageStatus, &seg3Cell3VoltageStatus, &seg3Cell4VoltageStatus, &seg3Cell5VoltageStatus, &seg3Cell6VoltageStatus, &seg3Cell7VoltageStatus, &seg3Cell8VoltageStatus, &seg3Cell9VoltageStatus, &seg3Cell10VoltageStatus, &seg3Cell11VoltageStatus, &seg3Cell12VoltageStatus},
		// 	{&seg4Cell1VoltageStatus, &seg4Cell2VoltageStatus, &seg4Cell3VoltageStatus, &seg4Cell4VoltageStatus, &seg4Cell5VoltageStatus, &seg4Cell6VoltageStatus, &seg4Cell7VoltageStatus, &seg4Cell8VoltageStatus, &seg4Cell9VoltageStatus, &seg4Cell10VoltageStatus, &seg4Cell11VoltageStatus, &seg4Cell12VoltageStatus},
		// 	{&seg5Cell1VoltageStatus, &seg5Cell2VoltageStatus, &seg5Cell3VoltageStatus, &seg5Cell4VoltageStatus, &seg5Cell5VoltageStatus, &seg5Cell6VoltageStatus, &seg5Cell7VoltageStatus, &seg5Cell8VoltageStatus, &seg5Cell9VoltageStatus, &seg5Cell10VoltageStatus, &seg5Cell11VoltageStatus, &seg5Cell12VoltageStatus},
		// 	{&seg6Cell1VoltageStatus, &seg6Cell2VoltageStatus, &seg6Cell3VoltageStatus, &seg6Cell4VoltageStatus, &seg6Cell5VoltageStatus, &seg6Cell6VoltageStatus, &seg6Cell7VoltageStatus, &seg6Cell8VoltageStatus, &seg6Cell9VoltageStatus, &seg6Cell10VoltageStatus, &seg6Cell11VoltageStatus, &seg6Cell12VoltageStatus},
		// 	{&seg7Cell1VoltageStatus, &seg7Cell2VoltageStatus, &seg7Cell3VoltageStatus, &seg7Cell4VoltageStatus, &seg7Cell5VoltageStatus, &seg7Cell6VoltageStatus, &seg7Cell7VoltageStatus, &seg7Cell8VoltageStatus, &seg7Cell9VoltageStatus, &seg7Cell10VoltageStatus, &seg7Cell11VoltageStatus, &seg7Cell12VoltageStatus}
		// };

		// static U8_CAN_STRUCT *cellTempStatusParams[NUM_BMBS_IN_ACCUMULATOR][NUM_CELLS_PER_BMB] =
		// {
		// 	{&seg1Cell1TempStatus, &seg1Cell2TempStatus, &seg1Cell3TempStatus, &seg1Cell4TempStatus, &seg1Cell5TempStatus, &seg1Cell6TempStatus, &seg1Cell7TempStatus, &seg1Cell8TempStatus, &seg1Cell9TempStatus, &seg1Cell10TempStatus, &seg1Cell11TempStatus, &seg1Cell12TempStatus},
		// 	{&seg2Cell1TempStatus, &seg2Cell2TempStatus, &seg2Cell3TempStatus, &seg2Cell4TempStatus, &seg2Cell5TempStatus, &seg2Cell6TempStatus, &seg2Cell7TempStatus, &seg2Cell8TempStatus, &seg2Cell9TempStatus, &seg2Cell10TempStatus, &seg2Cell11TempStatus, &seg2Cell12TempStatus},
		// 	{&seg3Cell1TempStatus, &seg3Cell2TempStatus, &seg3Cell3TempStatus, &seg3Cell4TempStatus, &seg3Cell5TempStatus, &seg3Cell6TempStatus, &seg3Cell7TempStatus, &seg3Cell8TempStatus, &seg3Cell9TempStatus, &seg3Cell10TempStatus, &seg3Cell11TempStatus, &seg3Cell12TempStatus},
		// 	{&seg4Cell1TempStatus, &seg4Cell2TempStatus, &seg4Cell3TempStatus, &seg4Cell4TempStatus, &seg4Cell5TempStatus, &seg4Cell6TempStatus, &seg4Cell7TempStatus, &seg4Cell8TempStatus, &seg4Cell9TempStatus, &seg4Cell10TempStatus, &seg4Cell11TempStatus, &seg4Cell12TempStatus},
		// 	{&seg5Cell1TempStatus, &seg5Cell2TempStatus, &seg5Cell3TempStatus, &seg5Cell4TempStatus, &seg5Cell5TempStatus, &seg5Cell6TempStatus, &seg5Cell7TempStatus, &seg5Cell8TempStatus, &seg5Cell9TempStatus, &seg5Cell10TempStatus, &seg5Cell11TempStatus, &seg5Cell12TempStatus},
		// 	{&seg6Cell1TempStatus, &seg6Cell2TempStatus, &seg6Cell3TempStatus, &seg6Cell4TempStatus, &seg6Cell5TempStatus, &seg6Cell6TempStatus, &seg6Cell7TempStatus, &seg6Cell8TempStatus, &seg6Cell9TempStatus, &seg6Cell10TempStatus, &seg6Cell11TempStatus, &seg6Cell12TempStatus},
		// 	{&seg7Cell1TempStatus, &seg7Cell2TempStatus, &seg7Cell3TempStatus, &seg7Cell4TempStatus, &seg7Cell5TempStatus, &seg7Cell6TempStatus, &seg7Cell7TempStatus, &seg7Cell8TempStatus, &seg7Cell9TempStatus, &seg7Cell10TempStatus, &seg7Cell11TempStatus, &seg7Cell12TempStatus}
		// };

		static uint32_t lastHighFreqGcanUpdate = 0;
		if((HAL_GetTick() - lastHighFreqGcanUpdate) >= 25)
		{
			lastHighFreqGcanUpdate = HAL_GetTick();

			update_and_queue_param_float(&soeByOCV_percent, gcanTaskInputData.currentSenseData.soc.soeByOcv * 100.0f);
			update_and_queue_param_float(&soeByCoulombCounting_percent, gcanTaskInputData.currentSenseData.soc.soeByCoulombCounting * 100.0f);

			update_and_queue_param_float(&bmsAveBrickVoltage_V, gcanTaskInputData.bmbTaskData.avgBrickV);
			update_and_queue_param_float(&bmsMaxBrickVoltage_V, gcanTaskInputData.bmbTaskData.maxCellVoltage);
			update_and_queue_param_float(&bmsMinBrickVoltage_V, gcanTaskInputData.bmbTaskData.minCellVoltage);

			update_and_queue_param_float(&bmsAveBrickTemp_C, gcanTaskInputData.bmbTaskData.avgCellTemp);
			update_and_queue_param_float(&bmsMaxBrickTemp_C, gcanTaskInputData.bmbTaskData.maxCellTemp);
			update_and_queue_param_float(&bmsMinBrickTemp_C, gcanTaskInputData.bmbTaskData.minCellTemp);

			update_and_queue_param_float(&bmsAveBoardTemp_C, gcanTaskInputData.bmbTaskData.avgBoardTemp);
			update_and_queue_param_float(&bmsMaxBoardTemp_C, gcanTaskInputData.bmbTaskData.maxBoardTemp);
			update_and_queue_param_float(&bmsMinBoardTemp_C, gcanTaskInputData.bmbTaskData.minBoardTemp);
		}

		// // Log gcan variables across the alloted time period in data chunks
		// static uint32_t lastGcanUpdate = 0;
		// if((HAL_GetTick() - lastGcanUpdate) >= GOPHER_CAN_LOGGING_PERIOD_MS)
		// {
		// 	lastGcanUpdate = HAL_GetTick();

		// 	// Log the accumulator data according to the current GCAN logging state
		// 	static uint8_t gcanUpdateState = 0;
		// 	switch (gcanUpdateState)
		// 	{
		// 		case GCAN_SEGMENT_1:
		// 		case GCAN_SEGMENT_2:
		// 		case GCAN_SEGMENT_3:
		// 		case GCAN_SEGMENT_4:
		// 		case GCAN_SEGMENT_5:
		// 		case GCAN_SEGMENT_6:
		// 		case GCAN_SEGMENT_7:
		// 		case GCAN_SEGMENT_8:
					
		// 			update_and_queue_param_float(cellVoltageStatsParams[gcanUpdateState][0], gBms.bmb[gcanUpdateState].segmentV);
		// 			update_and_queue_param_float(cellVoltageStatsParams[gcanUpdateState][1], gBms.bmb[gcanUpdateState].avgBrickV);
		// 			update_and_queue_param_float(cellVoltageStatsParams[gcanUpdateState][2], gBms.bmb[gcanUpdateState].maxBrickV);
		// 			update_and_queue_param_float(cellVoltageStatsParams[gcanUpdateState][3], gBms.bmb[gcanUpdateState].minBrickV);
					

		// 			for (int32_t i = 0; i < NUM_CELLS_PER_BMB; i++)
		// 			{
		// 				update_and_queue_param_float(cellVoltageParams[gcanUpdateState][i], gBms.bmb[gcanUpdateState].brickV[i]);
		// 				update_and_queue_param_float(cellTempParams[gcanUpdateState][i], gBms.bmb[gcanUpdateState].brickTemp[i]);

		// 				update_and_queue_param_float(cellVoltageStatusParams[gcanUpdateState][i], gBms.bmb[gcanUpdateState].brickVStatus[i]);
		// 				update_and_queue_param_float(cellTempStatusParams[gcanUpdateState][i], gBms.bmb[gcanUpdateState].brickTempStatus[i]);
		// 			}

		// 			for (int32_t i = 0; i < NUM_BOARD_TEMP_PER_BMB; i++)
		// 			{
		// 				update_and_queue_param_float(boardTempParams[gcanUpdateState][i], gBms.bmb[gcanUpdateState].boardTemp[i]);
		// 			}
		// 			break;
				
		// 		case GCAN_CELL_TEMP_STATS:
		// 			for (int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
		// 			{
		// 				update_and_queue_param_float(cellTempStatsParams[i][0], gBms.bmb[i].avgBrickTemp);
		// 				update_and_queue_param_float(cellTempStatsParams[i][1], gBms.bmb[i].maxBrickTemp);
		// 				update_and_queue_param_float(cellTempStatsParams[i][2], gBms.bmb[i].minBrickTemp);
		// 			}

		// 			update_and_queue_param_u8(&amsFault_state, gBms.amsFaultStatus);
		// 			update_and_queue_param_u8(&bspdFault_state, gBms.bspdFaultStatus);

		// 			break;

		// 		case GCAN_BOARD_TEMP_STATS:
		// 			for (int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
		// 			{
		// 				update_and_queue_param_float(boardTempStatsParams[i][0], gBms.bmb[i].avgBoardTemp);
		// 				update_and_queue_param_float(boardTempStatsParams[i][1], gBms.bmb[i].maxBoardTemp);
		// 				update_and_queue_param_float(boardTempStatsParams[i][2], gBms.bmb[i].minBoardTemp);
		// 			}

		// 			update_and_queue_param_u8(&imdFault_state, gBms.imdFaultStatus);
		// 			update_and_queue_param_u8(&imdFaultInfo_state, gBms.imdState);

		// 			break;

		// 		case GCAN_ALERTS_AND_INFO:
		// 			// Multiply SOE values by 100 to convert to percent before sending over gcan
		// 			// SOE values sent over gcan maintain 2 values after decimal place of percentage. Ex: 98.34% 
		// 			update_and_queue_param_u8(&bmsNumActiveAlerts_state, displayData.numActiveAlerts);
		// 			update_and_queue_param_u8(&bmsCurrAlertIndex_state, displayData.currAlertIndex);
		// 			update_and_queue_param_u8(&bmsAlertMessage_state, displayData.alertMessage);
		// 			update_and_queue_param_u8(&bmsCurrAlertIsLatched_state, displayData.currAlertIsLatched);

		// 			// Cycle through all alerts
		// 			for (uint32_t i = 0; i < NUM_GSENSE_ALERTS; i++)
		// 			{
		// 				Alert_S* alert = alerts[i];

		// 				// Triggers only if alert is active
		// 				if (getAlertStatus(alert) == ALERT_SET)
		// 				{
		// 					update_and_queue_param_u8(alertParams[i], 1);
		// 				} else {
		// 					update_and_queue_param_u8(alertParams[i], 0);
		// 				}
		// 			}
		// 			break;
		// 		default:
		// 			gcanUpdateState = 0;
		// 			break;
		// 	}

		// 	// Cycle Gcan state and wrap to 0 if needed
		// 	gcanUpdateState++;
		// 	gcanUpdateState %= NUM_GCAN_STATES;

		// 	service_can_tx(&hcan2);
		// }
	}
    
}