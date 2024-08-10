/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "bmbUpdateTask.h"
#include "main.h"
#include "cmsis_os.h"
#include "lookupTable.h"
#include <stdio.h>
#include "debug.h"
#include "alerts.h"
#include "packData.h"
#include "bmbUtils.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define ADC_RD                  (1 << 8)
#define ADC_DCP                 (1 << 4)
#define ADC_CONT                (1 << 7)
#define ADC_OW_EVEN             (1)
#define ADC_OW_ODD              (1 << 1)
#define CMD_START_CADC          0x0260
#define CMD_START_SADC          0x0168

#define CMD_START_AUX_ADC       0x0410  

#define WR_CFG_REG_A            0x0001
#define WR_CFG_REG_B            0x0024

#define READ_VOLT_REG_A         0x0004
#define READ_VOLT_REG_B         0x0006
#define READ_VOLT_REG_C         0x0008
#define READ_VOLT_REG_D         0x000A
#define READ_VOLT_REG_E         0x0009
#define READ_VOLT_REG_F         0x000B

#define READ_AVG_VOLT_REG_A     0x0044
#define READ_AVG_VOLT_REG_B     0x0046
#define READ_AVG_VOLT_REG_C     0x0048
#define READ_AVG_VOLT_REG_D     0x004A
#define READ_AVG_VOLT_REG_E     0x0049
#define READ_AVG_VOLT_REG_F     0x004B

#define READ_FILT_VOLT_REG_A    0x0012
#define READ_FILT_VOLT_REG_B    0x0013
#define READ_FILT_VOLT_REG_C    0x0014
#define READ_FILT_VOLT_REG_D    0x0015
#define READ_FILT_VOLT_REG_E    0x0016
#define READ_FILT_VOLT_REG_F    0x0017

#define READ_SADC_VOLT_REG_A    0x0003
#define READ_SADC_VOLT_REG_B    0x0005
#define READ_SADC_VOLT_REG_C    0x0007
#define READ_SADC_VOLT_REG_D    0x000D
#define READ_SADC_VOLT_REG_E    0x000E
#define READ_SADC_VOLT_REG_F    0x000F

#define NUM_VOLT_REG            6

#define READ_AUX_REG_A          0x0019
#define READ_AUX_REG_B          0x001A
#define READ_AUX_REG_C          0x001B
#define NUM_AUX_REG             3

#define READ_STAT_REG_A         0x0030

#define WR_PWM_A                0x0020
#define WR_PWM_B                0x0021

#define NUM_PWM_REG_A           12
#define NUM_PWM_REG_B           4

#define PWM_DUTY_MASK           0xF

#define CMD_MUTE_DIS            0x0028
#define CMD_UNMUTE_DIS          0x0029

#define TEMPS_PER_MUX           8
#define CELLS_PER_REG           3
#define CELL_REG_SIZE           REGISTER_SIZE_BYTES / CELLS_PER_REG     

#define ADC_RESOLUTION          0.00015f
#define ADC_OFFSET_VOLT         1.5f

#define DIE_TEMP_RESOLUTION     0.02f
#define DIE_TEMP_OFFSET_DEGC    -73.0f

#define TEST_REG                0x002C

#define SADC_DIAG_PERIOD_MS     100
#define SADC_DIAG_PERIOD_CYC    (SADC_DIAG_PERIOD_MS < BMB_UPDATE_TASK_PERIOD_MS) ? (1) : (SADC_DIAG_PERIOD_MS / BMB_UPDATE_TASK_PERIOD_MS)

#define BALANCE_DIAG_PERIOD_MS  10000
#define BALANCE_DIAG_PERIOD_CYC (BALANCE_DIAG_PERIOD_MS < BMB_UPDATE_TASK_PERIOD_MS) ? (1) : (BALANCE_DIAG_PERIOD_MS / BMB_UPDATE_TASK_PERIOD_MS)

// The number of values contained in a lookup table
#define LUT_SIZE 33

#define MAX_BRICK_VOLTAGE_READING 5.0f
#define MIN_BRICK_VOLTAGE_READING 0.0f

#define MAX_TEMP_SENSE_READING 	  120.0f
#define MIN_TEMP_SENSE_READING	  (-40.0f)

// The minimum voltage that we can bleed to
#define MIN_BLEED_TARGET_VOLTAGE_V 			3.5f

// Max allowable voltage difference between bricks for balancing
#define BALANCE_THRESHOLD_V					0.001f

// The maximum cell temperature where bleeding is allowed
#define MAX_CELL_TEMP_BLEEDING_ALLOWED_C	55.0f

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    MUX_STATE_0 = 0,
    MUX_STATE_1,
    NUM_MUX_STATES
} MUX_STATE_E;

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

extern volatile bool chargerConnected;

uint16_t readVoltReg[NUM_VOLT_REG] =
{
    READ_VOLT_REG_A, READ_VOLT_REG_B,
    READ_VOLT_REG_C, READ_VOLT_REG_D,
    READ_VOLT_REG_E, READ_VOLT_REG_F,
};

uint16_t readAvgVoltReg[NUM_VOLT_REG] =
{
    READ_AVG_VOLT_REG_A, READ_AVG_VOLT_REG_B,
    READ_AVG_VOLT_REG_C, READ_AVG_VOLT_REG_D,
    READ_AVG_VOLT_REG_E, READ_AVG_VOLT_REG_F,
};

uint16_t readFiltVoltReg[NUM_VOLT_REG] =
{
    READ_FILT_VOLT_REG_A, READ_FILT_VOLT_REG_B,
    READ_FILT_VOLT_REG_C, READ_FILT_VOLT_REG_D,
    READ_FILT_VOLT_REG_E, READ_FILT_VOLT_REG_F,
};

uint16_t readSadcVoltReg[NUM_VOLT_REG] =
{
    READ_SADC_VOLT_REG_A, READ_SADC_VOLT_REG_B,
    READ_SADC_VOLT_REG_C, READ_SADC_VOLT_REG_D,
    READ_SADC_VOLT_REG_E, READ_SADC_VOLT_REG_F,
};

uint16_t readAuxReg[NUM_AUX_REG] =
{
    READ_AUX_REG_A,
    READ_AUX_REG_B,
    READ_AUX_REG_C
};

const float temperatureArray[LUT_SIZE] =
			{120, 115, 110, 105, 100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35,
			 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40};

const float cellTempVoltageArray[LUT_SIZE] =
			{0.2919345638, 0.324518624, 0.3612309495, 0.402585263, 0.4491372839, 0.5014774689, 0.5602182763, 0.6259742709, 0.699333326, 0.7808173945, 0.8708320017, 0.969604948, 1.077116849, 1.193029083, 1.316618141, 1.44672857, 1.581758437, 1.71969016, 1.85817452, 1.994666667, 2.126601617, 2.25158619, 2.367578393, 2.473026369, 2.566947369, 2.648940056, 2.719136612, 2.778110865, 2.826762844, 2.866199204, 2.897624362, 2.922251328, 2.941235685};

const float boardTempVoltageArray[LUT_SIZE] =
			{0.1741318359, 0.193533737, 0.2155192602, 0.2404628241, 0.2687907644, 0.3009857595, 0.3375901116, 0.3792069573, 0.4264981201, 0.480176881, 0.5409934732, 0.6097106855, 0.6870667304, 0.7737227417, 0.870193244, 0.976760075, 1.093373849, 1.219552143, 1.354289544, 1.496, 1.642514054, 1.791149899, 1.938865924, 2.082484747, 2.218959086, 2.345635446, 2.460468742, 2.562151894, 2.650145243, 2.724613713, 2.786297043, 2.836345972, 2.87615517};

LookupTable_S cellTempTable =  { .length = LUT_SIZE, .x = cellTempVoltageArray, .y = temperatureArray};
LookupTable_S boardTempTable = { .length = LUT_SIZE, .x = boardTempVoltageArray, .y = temperatureArray};

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static bool initBmbs();
static TRANSACTION_STATUS_E updateCellVoltages(BmbTaskOutputData_S* bmbData);
static TRANSACTION_STATUS_E updateCellTemps(Bmb_S* bmb);
static TRANSACTION_STATUS_E updateTestData(Bmb_S* bmb);
static void aggregateBmbData(Bmb_S* bmb);
static void aggregatePackData(BmbTaskOutputData_S* bmbData);

static TRANSACTION_STATUS_E updateSADC(BmbTaskOutputData_S* bmbData);
static void checkAdcMismatch(BmbTaskOutputData_S* bmbData);
static void checkOpenWire(BmbTaskOutputData_S* bmbData);
static TRANSACTION_STATUS_E updateBalanceSwitches(BmbTaskOutputData_S* bmbData);
static TRANSACTION_STATUS_E runSwitchPinStateMachine(BmbTaskOutputData_S* bmbData, bool balancingEnabled);

static void runBmbAlertMonitor(BmbTaskOutputData_S* bmbData);

/* ==================================================================== */
/* ============================== MACROS ============================== */
/* ==================================================================== */

#define HANDLE_BMB_ERROR(error) \
    if(error != TRANSACTION_SUCCESS) { \
        if(error == TRANSACTION_SPI_ERROR) \
        { \
            Debug("SPI Failure, reseting STM32...\n"); \
            HAL_NVIC_SystemReset(); \
        } \
        else if(error == TRANSACTION_POR_ERROR) \
        { \
            Debug("Power reset detected, reinitializing...\n"); \
            bmbsInit = initBmbs(); \
            return; \
        } \
        else if(error == TRANSACTION_CRC_ERROR) \
        { \
            Debug("Chain break!\n"); \
            // return; \
        } \
        else if(error == TRANSACTION_COMMAND_COUNTER_ERROR) \
        { \
            Debug("Command Counter Error!\n"); \
            return; \
        } \
        else if(error == TRANSACTION_WRITE_REJECT) \
        { \
            Debug("Write Rejected!\n"); \
            return; \
        } \
        else \
        { \
            Debug("Unknown Transaction Error\n"); \
            return; \
        } \
    }

// Convert 16bit ADC reading to voltage
#define CONVERT_16_BIT_ADC(rawAdc) (((int16_t)(rawAdc) * ADC_RESOLUTION) + ADC_OFFSET_VOLT)

// Convert 16bit ADC reading to die temperature
#define CONVERT_DIE_TEMP(rawAdc) (((int16_t)(rawAdc) * DIE_TEMP_RESOLUTION) + DIE_TEMP_OFFSET_DEGC)

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static bool initBmbs()
{
    bool initSuccess = true;

    // uint8_t data[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    // initSuccess |= (writeAll(WR_CFG_REG_A, NUM_BMBS_IN_ACCUMULATOR, data) != TRANSACTION_SUCCESS);
    initSuccess &= (commandAll((CMD_START_CADC | ADC_CONT), NUM_BMBS_IN_ACCUMULATOR) == TRANSACTION_SUCCESS);
    initSuccess &= (commandAll(CMD_START_AUX_ADC, NUM_BMBS_IN_ACCUMULATOR) == TRANSACTION_SUCCESS);

    if(!initSuccess)
    {
        Debug("BMBs failed to initialize!\n");
        return false;
    }
    Debug("BMB initialization successful!\n");
    return true;
}

static TRANSACTION_STATUS_E updateCellTemps(Bmb_S* bmb)
{
    // Hold current state of off-chip analog MUX 
    // static MUX_STATE_E muxState = MUX_STATE_0;


    // Create a register buffer for read transactions
    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    TRANSACTION_STATUS_E msgStatus;

    // Read Aux A
    // Clear register buffer
    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // Read aux register from all BMBs
    msgStatus = readAll(readAuxReg[0], NUM_BMBS_IN_ACCUMULATOR, registerData);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }

    // Sort through aux registers and populate BMB struct cell temps
    for(int32_t j = 0; j < CELLS_PER_REG-1; j++)
    {
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            // Extract 16bit ADC from register
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

            // Convert ADC to temp and populate bmb struct
            float cellTemp = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
            bmb[k].cellTemp[((j * 2) + 3)] = cellTemp;

            if(cellTemp > 115.0f || cellTemp < -35.0f)
            {
                bmb[k].cellTempStatus[((j * 2) + 3)] = BAD;
            }
            else
            {
                bmb[k].cellTempStatus[((j * 2) + 3)] = GOOD;
            }
        }
    }

    // Read Aux B
    // Clear register buffer
    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // Read aux register from all BMBs
    msgStatus = readAll(readAuxReg[1], NUM_BMBS_IN_ACCUMULATOR, registerData);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }

    // Sort through aux registers and populate BMB struct cell temps
    for(int32_t j = 0; j < CELLS_PER_REG; j++)
    {
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            // Extract 16bit ADC from register
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

            float cellTemp = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
            bmb[k].cellTemp[((j * 2) + 7)] = cellTemp;

            if(cellTemp > 115.0f || cellTemp < -35.0f)
            {
                bmb[k].cellTempStatus[((j * 2) + 7)] = BAD;
            }
            else
            {
                bmb[k].cellTempStatus[((j * 2) + 7)] = GOOD;
            }
        }
    }

    // Read Aux C
    // Clear register buffer
    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // Read aux register from all BMBs
    msgStatus = readAll(readAuxReg[2], NUM_BMBS_IN_ACCUMULATOR, registerData);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }

    // Sort through aux registers and populate BMB struct cell temps
    for(int32_t j = 0; j < CELLS_PER_REG-1; j++)
    {
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            // Extract 16bit ADC from register
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

            // Convert ADC to temp and populate bmb struct 
            float cellTemp = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
            bmb[k].cellTemp[((j * 2) + 13)] = cellTemp;

            if(cellTemp > 115.0f || cellTemp < -35.0f)
            {
                bmb[k].cellTempStatus[((j * 2) + 13)] = BAD;
            }
            else
            {
                bmb[k].cellTempStatus[((j * 2) + 13)] = GOOD;
            }
        }
    }

    // Extract final ADC value from register (board temp)
    for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    {
        // Extract 16bit ADC from register
        uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (2 * CELL_REG_SIZE)];
        uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (2 * CELL_REG_SIZE) + 1];
        uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

        // Convert ADC to temp and populate bmb struct
        float boardTemp = lookup(CONVERT_16_BIT_ADC(rawAdc), &boardTempTable);
        bmb[k].boardTemp = boardTemp;

        if(boardTemp > 115.0f || boardTemp < -35.0f)
        {
            bmb[k].boardTempStatus = BAD;
        }
        else
        {
            bmb[k].boardTempStatus = GOOD;
        }
    }

    // Read Stat A
    // Clear register buffer
    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // Read stat register from all BMBs
    msgStatus = readAll(READ_STAT_REG_A, NUM_BMBS_IN_ACCUMULATOR, registerData);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }

    // Sort through stat registers and populate BMB struct die temps
    for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    {
        // Extract 16bit ADC from register
        uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (CELL_REG_SIZE)];
        uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (CELL_REG_SIZE) + 1];
        uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

        // Convert ADC to temp and populate bmb struct 
        bmb[k].dieTemp = CONVERT_DIE_TEMP(rawAdc);
        bmb[k].dieTempStatus = GOOD;
    }

    // for(int32_t i = 0; i < NUM_AUX_REG-1; i++)
    // {
    //     // Clear register buffer
    //     memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    //     // Read aux register from all BMBs
    //     msgStatus = readAll(readAuxReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
    //     if(msgStatus != TRANSACTION_SUCCESS)
    //     {
    //         return msgStatus;
    //     }

    //     // Sort through aux registers and populate BMB struct cell temps
    //     for(int32_t j = 0; j < CELLS_PER_REG; j++)
    //     {
    //         for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    //         {
    //             // Extract 16bit ADC from register
    //             uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
    //             uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
    //             uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

    //             // Convert ADC to temp and populate bmb struct 
    //             bmb[k].cellTemp[(muxState) + 2 * ((i * CELLS_PER_REG) + j)] = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
    //             bmb[k].cellTempStatus[(muxState) + 2 * ((i * CELLS_PER_REG) + j)] = GOOD;
    //         }
    //     }
    // }

    // // Clear register buffer
    // memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // // Read aux register from all BMBs
    // msgStatus = readAll(readAuxReg[2], NUM_BMBS_IN_ACCUMULATOR, registerData);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }

    // // Sort through aux register and populate BMB struct cell temps
    // for(int32_t j = 0; j < CELLS_PER_REG-1; j++)
    // {
    //     for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    //     {
    //         // Extract 16bit ADC from register
    //         uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
    //         uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
    //         uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

    //         // Convert ADC to temp and populate bmb struct 
    //         bmb[k].cellTemp[(muxState) + 2 * ((2 * CELLS_PER_REG) + j)] = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
    //         bmb[k].cellTempStatus[(muxState) + 2 * ((2 * CELLS_PER_REG) + j)] = GOOD;
    //     }
    // }

    // // Extract final ADC value from register (board temp)
    // for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    // {
    //     // Extract 16bit ADC from register
    //     uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (2 * CELL_REG_SIZE)];
    //     uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (2 * CELL_REG_SIZE) + 1];
    //     uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

    //     // Convert ADC to temp and populate bmb struct
    //     bmb[k].boardTemp = lookup(CONVERT_16_BIT_ADC(rawAdc), &boardTempTable);
    //     bmb[k].boardTempStatus = GOOD;
    // }

    // Cycle mux state
    // muxState++;
    // muxState %= NUM_MUX_STATES;

    // uint8_t data[6] = {0x01, 0x00, 0x00, 0xFF, 0x00, 0x00};
    // data[4] = ((uint8_t)muxState << 1) | (0x01);
    // msgStatus = writeAll(WR_CFG_REG_A, NUM_BMBS_IN_ACCUMULATOR, data);
    // if((msgStatus != TRANSACTION_SUCCESS) && (msgStatus != TRANSACTION_CRC_ERROR) && (msgStatus != TRANSACTION_WRITE_REJECT) && (msgStatus != TRANSACTION_COMMAND_COUNTER_ERROR))
    // {
    //     return msgStatus;
    // }

    // Start Aux ADC conversion
    msgStatus = commandAll(CMD_START_AUX_ADC, NUM_BMBS_IN_ACCUMULATOR);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E updateCellVoltages(BmbTaskOutputData_S* bmbData)
{
    if((bmbData->sPinState != SADC_OW_EVEN) && (bmbData->sPinState != SADC_OW_ODD))
    {
        TRANSACTION_STATUS_E msgStatus;
        uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];

        //Mute S-pin
        msgStatus = commandAll(CMD_MUTE_DIS, NUM_BMBS_IN_ACCUMULATOR);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }

        vTaskDelay(2);

        // Update cell voltages from raw CADC registers
        for(int32_t i = 0; i < NUM_VOLT_REG-1; i++)
        {
            memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
            msgStatus = readAll(readVoltReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
            if(msgStatus != TRANSACTION_SUCCESS)
            {
                return msgStatus;
            }
            for(int32_t j = 0; j < CELLS_PER_REG; j++)
            {
                for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
                {
                    uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
                    uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
                    uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
                    bmbData->bmb[k].cellVoltage[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                    bmbData->bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = GOOD;
                }
            }
        }

        memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
        msgStatus = readAll(readVoltReg[5], NUM_BMBS_IN_ACCUMULATOR, registerData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
            bmbData->bmb[k].cellVoltage[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
            bmbData->bmb[k].cellVoltageStatus[(5 * CELLS_PER_REG)] = GOOD;
        }

        // Update avg cell voltages from avg CADC registers
        for(int32_t i = 0; i < NUM_VOLT_REG-1; i++)
        {
            memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
            msgStatus = readAll(readAvgVoltReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
            if(msgStatus != TRANSACTION_SUCCESS)
            {
                return msgStatus;
            }
            for(int32_t j = 0; j < CELLS_PER_REG; j++)
            {
                for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
                {
                    uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
                    uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
                    uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
                    bmbData->bmb[k].cellVoltageAvg[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                    bmbData->bmb[k].cellVoltageAvgStatus[(i * CELLS_PER_REG) + j] = GOOD;
                }
            }
        }

        memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
        msgStatus = readAll(readAvgVoltReg[5], NUM_BMBS_IN_ACCUMULATOR, registerData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
            bmbData->bmb[k].cellVoltageAvg[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
            bmbData->bmb[k].cellVoltageAvgStatus[(5 * CELLS_PER_REG)] = GOOD;
        }

        // Update filtered cell voltages from filtered CADC registers
        for(int32_t i = 0; i < NUM_VOLT_REG-1; i++)
        {
            memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
            msgStatus = readAll(readFiltVoltReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
            if(msgStatus != TRANSACTION_SUCCESS)
            {
                return msgStatus;
            }
            for(int32_t j = 0; j < CELLS_PER_REG; j++)
            {
                for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
                {
                    uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
                    uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
                    uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
                    bmbData->bmb[k].cellVoltageFiltered[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                    bmbData->bmb[k].cellVoltageFilteredStatus[(i * CELLS_PER_REG) + j] = GOOD;
                }
            }
        }

        memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
        msgStatus = readAll(readFiltVoltReg[5], NUM_BMBS_IN_ACCUMULATOR, registerData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
            bmbData->bmb[k].cellVoltageFiltered[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
            bmbData->bmb[k].cellVoltageFilteredStatus[(5 * CELLS_PER_REG)] = GOOD;
        }

        msgStatus = commandAll(CMD_START_CADC | ADC_CONT | ADC_RD, NUM_BMBS_IN_ACCUMULATOR);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }

        //Unmute S-pin
        msgStatus = commandAll(CMD_UNMUTE_DIS, NUM_BMBS_IN_ACCUMULATOR);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }

        return msgStatus;
    }
    return TRANSACTION_SUCCESS;    
}

static void aggregateBmbData(Bmb_S* bmb)
{
    // Iterate through brick voltages
	for (int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
	{
		Bmb_S* pBmb = &bmb[i];
		float maxBrickVoltage = MIN_VOLTAGE_SENSOR_VALUE_V;
		float minBrickVoltage = MAX_VOLTAGE_SENSOR_VALUE_V;
		float sumV	= 0.0f;
		uint32_t numGoodBrickV = 0;

		float maxBrickTemp = MIN_TEMP_SENSOR_VALUE_C;
		float minBrickTemp = MAX_TEMP_SENSOR_VALUE_C;
		float brickTempSum = 0.0f;
		uint32_t numGoodBrickTemp = 0;

		// Aggregate brick voltage and temperature data
		for (int32_t j = 0; j < NUM_CELLS_PER_BMB; j++)
		{
			// Only update stats if sense status is good
			if ((pBmb->cellVoltageStatus[j] == GOOD) && (!pBmb->openWire[j]))
			{
				float brickV = pBmb->cellVoltage[j];

				if (brickV > maxBrickVoltage)
				{
					maxBrickVoltage = brickV;
				}
				if (brickV < minBrickVoltage)
				{
					minBrickVoltage = brickV;
				}
				numGoodBrickV++;
				sumV += brickV;
			}

			// Only update stats if sense status is good
			if (pBmb->cellTempStatus[j] == GOOD)
			{
				float brickTemp = pBmb->cellTemp[j];

				if (brickTemp > maxBrickTemp)
				{
					maxBrickTemp = brickTemp;
				}
				if (brickTemp < minBrickTemp)
				{
					minBrickTemp = brickTemp;
				}
				numGoodBrickTemp++;
				brickTempSum += brickTemp;
			}
		}

		// Update BMB statistics
		pBmb->maxBrickVoltage = maxBrickVoltage;
		pBmb->minBrickVoltage = minBrickVoltage;
		pBmb->sumBrickVoltage = sumV;
		pBmb->avgBrickVoltage = (numGoodBrickV == 0) ? pBmb->avgBrickVoltage : sumV / numGoodBrickV;
		pBmb->numBadBrickV = NUM_CELLS_PER_BMB - numGoodBrickV;

		pBmb->maxBrickTemp = maxBrickTemp;
		pBmb->minBrickTemp = minBrickTemp;
		pBmb->avgBrickTemp = (numGoodBrickTemp == 0) ? pBmb->avgBrickTemp :  brickTempSum / numGoodBrickTemp;
		pBmb->numBadBrickTemp = NUM_CELLS_PER_BMB - numGoodBrickTemp;

	// 	pBmb->maxBoardTemp = maxBoardTemp;
	// 	pBmb->minBoardTemp = minBoardTemp;
	// 	pBmb->avgBoardTemp = (numGoodBoardTemp == 0) ? pBmb->avgBoardTemp : boardTempSum / numGoodBoardTemp;
	// 	pBmb->numBadBoardTemp = NUM_BOARD_TEMP_PER_BMB - numGoodBoardTemp;
	}
}

static void aggregatePackData(BmbTaskOutputData_S* bmbData)
{
    BmbTaskOutputData_S* pBms = bmbData;

	// Update BMB level stats
	aggregateBmbData(bmbData->bmb);

	float maxCellVoltage	   = MIN_BRICK_VOLTAGE_READING;
	float minCellVoltage	   = MAX_BRICK_VOLTAGE_READING;
	float avgCellVoltageSum = 0.0f;
	float accumulatorVSum = 0.0f;

	float maxCellTemp    = MIN_TEMP_SENSE_READING;
	float minCellTemp 	  = MAX_TEMP_SENSE_READING;
	float avgCellTempSum = 0.0f;

	float maxBoardTemp    = MIN_TEMP_SENSE_READING;
	float minBoardTemp 	  = MAX_TEMP_SENSE_READING;
	float avgBoardTempSum = 0.0f;
    uint32_t numGoodBoardTemp = 0;

	for (int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
	{
		Bmb_S* pBmb = &pBms->bmb[i];

		if (pBmb->maxBrickVoltage > maxCellVoltage)
		{
			maxCellVoltage = pBmb->maxBrickVoltage;
		}
		if (pBmb->minBrickVoltage < minCellVoltage)
		{
			minCellVoltage = pBmb->minBrickVoltage;
		}
        
        for (uint16_t i = 0; i < NUM_CELLS_PER_BMB; i++) {

            if (pBmb->cellTemp[i] > maxCellTemp)
            {
                maxCellTemp = pBmb->cellTemp[i];
            }
            if (pBmb->cellTemp[i] < minCellTemp)
            {
                minCellTemp = pBmb->cellTemp[i];
            }

        }

		if (pBmb->maxBrickVoltage > maxCellVoltage)
		{
			maxCellVoltage = pBmb->maxBrickVoltage;
		}
		if (pBmb->minBrickVoltage < minCellVoltage)
		{
			minCellVoltage = pBmb->minBrickVoltage;
		}

        avgCellVoltageSum += pBmb->avgBrickVoltage;
		accumulatorVSum += pBmb->sumBrickVoltage;
		avgCellTempSum += pBmb->avgBrickTemp;

        if(pBmb->boardTempStatus == GOOD)
        {
            numGoodBoardTemp++;
            if (pBmb->boardTemp > maxBoardTemp)
            {
                maxBoardTemp = pBmb->boardTemp;
            }
            if (pBmb->boardTemp < minBoardTemp)
            {
                minBoardTemp = pBmb->boardTemp;
            }
            avgBoardTempSum += pBmb->boardTemp;
        }
		
	}
	pBms->accumulatorVoltage = accumulatorVSum;
	pBms->maxCellVoltage = maxCellVoltage;
	pBms->minCellVoltage = minCellVoltage;
	pBms->avgBrickV = avgCellVoltageSum / NUM_BMBS_IN_ACCUMULATOR;
	pBms->maxCellTemp = maxCellTemp;
	pBms->minCellTemp = minCellTemp;
	pBms->avgCellTemp = avgCellTempSum / NUM_BMBS_IN_ACCUMULATOR;
	pBms->maxBoardTemp = maxBoardTemp;
	pBms->minBoardTemp = minBoardTemp;
    pBms->avgBoardTemp = (numGoodBoardTemp == 0) ? pBms->avgBoardTemp : avgBoardTempSum / numGoodBoardTemp;
}


static TRANSACTION_STATUS_E updateSADC(BmbTaskOutputData_S* bmbData)
{
    TRANSACTION_STATUS_E msgStatus = TRANSACTION_SUCCESS;

    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];

    // Update SADC cell voltages from SADC registers
    for(int32_t i = 0; i < NUM_VOLT_REG-1; i++)
    {
        memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
        msgStatus = readAll(readSadcVoltReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        for(int32_t j = 0; j < CELLS_PER_REG; j++)
        {
            for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
            {
                uint16_t cellIndex = (i * CELLS_PER_REG) + j;
                uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
                uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
                uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
                bmbData->bmb[k].cellVoltageRedundant[cellIndex] = CONVERT_16_BIT_ADC(rawAdc);
                bmbData->bmb[k].cellVoltageRedundantStatus[cellIndex] = GOOD;
            }
        }
    }

    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
    msgStatus = readAll(readSadcVoltReg[5], NUM_BMBS_IN_ACCUMULATOR, registerData);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }
    for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    {
        uint16_t cellIndex = 5 * CELLS_PER_REG;
        uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES)];
        uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + 1];
        uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
        bmbData->bmb[k].cellVoltageRedundant[cellIndex] = CONVERT_16_BIT_ADC(rawAdc);
        bmbData->bmb[k].cellVoltageRedundantStatus[cellIndex] = GOOD;
    }
    return msgStatus;
}

static void checkAdcMismatch(BmbTaskOutputData_S* bmbData)
{
    if(bmbData->sPinState == SADC_REDUNDANT)
    {
        for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
        {
            for(int32_t j = 0; j < NUM_CELLS_PER_BMB; j++)
            {
                if(fabs(bmbData->bmb[i].cellVoltage[j] - bmbData->bmb[i].cellVoltageRedundant[j]) > 0.2f)
                {
                    bmbData->bmb[i].adcMismatch[j] = true;
                }
                else
                {
                    bmbData->bmb[i].adcMismatch[j] = false;
                }
            }
        }
    }
}

static void checkOpenWire(BmbTaskOutputData_S* bmbData)
{
    if(bmbData->sPinState == SADC_OW_EVEN || bmbData->sPinState == SADC_OW_ODD)
    {
        uint32_t initIndex = 0;
        if(bmbData->sPinState == SADC_OW_EVEN)
        {
            initIndex = 1;
        }
        for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
        {
            for(int32_t j = initIndex; j < NUM_CELLS_PER_BMB; j+=2)
            {
                bmbData->bmb[i].openWire[j] = (bmbData->bmb[i].cellVoltageRedundant[j] < 2.0f);
                // if(bmbData->bmb[i].cellVoltageRedundant[j] < 2.0f)
                // {
                //     // bmbData->bmb[i].openAdcMask |= ((uint16_t)1<<j);
                //     bmbData->bmb[i].openWire[j] = true;
                // }
                // else
                // {
                //     // bmbData->bmb[i].openAdcMask &= ~((uint16_t)1<<j);
                //     bmbData->bmb[i].openWire[j] = false;
                // }
            }

            // for(int32_t j = 1; j < NUM_CELLS_PER_BMB; j++)
            // {
            //     bmbData->bmb[i].openWire[j] = (((bmbData->bmb[i].openAdcMask >> (j-1)) & 0x0003) == 0x0003);
            // }  
            // bmbData->bmb[i].openWire[0] = (((bmbData->bmb[i].openAdcMask) & 0x0003) == 0x0001);
            // bmbData->bmb[i].openWire[NUM_CELLS_PER_BMB] = (((bmbData->bmb[i].openAdcMask) & 0xC000) == 0x8000); 
        }
    }
}

static TRANSACTION_STATUS_E updateBalanceSwitches(BmbTaskOutputData_S* bmbData)
{
    // Determine minimum voltage across entire battery pack
	float bleedTargetVoltage = bmbData->minCellVoltage;

	// Ensure we don't overbleed the cells
	if (bleedTargetVoltage < MIN_BLEED_TARGET_VOLTAGE_V)
	{
		bleedTargetVoltage = MIN_BLEED_TARGET_VOLTAGE_V;
	}

    BmbTaskOutputData_S* pBms = bmbData;

    // Iterate through all BMBs and set bleed request
	for (int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
	{
        Bmb_S* pBmb = &pBms->bmb[i];
		// Iterate through all bricks and determine whether they should be bled or not
		for (int32_t j = 0; j < NUM_CELLS_PER_BMB; j++)
		{
			if (pBmb->cellVoltage[j] > bleedTargetVoltage + BALANCE_THRESHOLD_V)
			{
				pBmb->balSwRequested[j] = true;
			}
			else
			{
				pBmb->balSwRequested[j] = false;
			}
		}
	}

    // To determine cell balancing priority add all cells that need to be balanced to an array
	// Sort this array by cell voltage. We always want to balance the highest cell voltage. 
	// Iterate through the array starting with the highest voltage and enable balancing switch
	// if neighboring cells aren't being balanced. This is due to the circuit not allowing 
	// neighboring cells to be balanced. 
	for (int32_t bmbIdx = 0; bmbIdx < NUM_BMBS_IN_ACCUMULATOR; bmbIdx++)
	{
        Bmb_S* pBmb = &pBms->bmb[bmbIdx];
		uint32_t numBricksNeedBalancing = 0;
        Brick_S bricksToBalance[NUM_CELLS_PER_BMB];

        for (int32_t brickIdx = 0; brickIdx < NUM_CELLS_PER_BMB; brickIdx++)
        {
            // Add brick to list of bricks that need balancing if balancing requested, brick
            // isn't too hot, and the brick voltage is above the bleed threshold
            if (pBmb->balSwRequested[brickIdx] &&
                pBmb->cellTemp[brickIdx] < MAX_CELL_TEMP_BLEEDING_ALLOWED_C &&
                pBmb->cellVoltage[brickIdx] > MIN_BLEED_TARGET_VOLTAGE_V)
            {
                // Brick needs to be balanced, add to array
                bricksToBalance[numBricksNeedBalancing++] = (Brick_S) { .brickIdx = brickIdx, .brickV = pBmb->cellVoltage[brickIdx] };
            }
        }

		// Sort array of bricks that need balancing by their voltage
		insertionSort(bricksToBalance, numBricksNeedBalancing);
		// Clear all balance switches
		memset(pBmb->balSwEnabled, 0, NUM_CELLS_PER_BMB * sizeof(bool));

		for (int32_t i = numBricksNeedBalancing - 1; i >= 0; i--)
		{
			// For each brick that needs balancing ensure that the neighboring bricks aren't being bled
			Brick_S brick = bricksToBalance[i];
			int leftIdx = brick.brickIdx - 1;
			int rightIdx = brick.brickIdx + 1;
			bool leftNotBalancing = false;
			bool rightNotBalancing = false;
			if (leftIdx < 0)
			{
				leftNotBalancing = true;
			}
			else if (!pBmb->balSwEnabled[leftIdx])
			{
				leftNotBalancing = true;
			}

			if (rightIdx >= NUM_CELLS_PER_BMB)
			{
				rightNotBalancing = true;
			}
			else if (!pBmb->balSwEnabled[rightIdx])
			{
				rightNotBalancing = true;
			}

			if (leftNotBalancing && rightNotBalancing)
			{
				pBmb->balSwEnabled[brick.brickIdx] = true;
			}
		}
    }

    // uint8_t registerDataPwmA[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    // memset(registerDataPwmA, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // uint8_t registerDataPwmB[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    // memset(registerDataPwmB, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // for(int32_t i = 0; i < 1; i++)
    // {
    //     Bmb_S* pBmb = &pBms->bmb[i];

    //     for(int32_t j = 0; j < NUM_PWM_REG_A; j+=2)
    //     {
    //         if(pBmb->balSwEnabled[j])
    //         {
    //             registerDataPwmA[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + (j / 2)] |= PWM_DUTY_MASK; 
    //         }
    //         if(pBmb->balSwEnabled[j+1])
    //         {
    //             registerDataPwmA[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + (j / 2)] |= (((uint8_t)PWM_DUTY_MASK) << 4); 
    //         }
    //     }

    //     for(int32_t j = 0; j < NUM_PWM_REG_B; j+=2)
    //     {
    //         if(pBmb->balSwEnabled[j+NUM_PWM_REG_A])
    //         {
    //             registerDataPwmB[(REGISTER_SIZE_BYTES * i) + (j / 2)] |= PWM_DUTY_MASK; 
    //         }
    //         if(pBmb->balSwEnabled[j+NUM_PWM_REG_A+1])
    //         {
    //             registerDataPwmB[(REGISTER_SIZE_BYTES * i) + (j / 2)] |= (((uint8_t)PWM_DUTY_MASK) << 4); 
    //         }
    //     }
    // }

    // TRANSACTION_STATUS_E msgStatus;
    // msgStatus = writeAll(WR_PWM_A, NUM_BMBS_IN_ACCUMULATOR, registerDataPwmA);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    // msgStatus = writeAll(WR_PWM_B, NUM_BMBS_IN_ACCUMULATOR, registerDataPwmB);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    // return msgStatus;
    // // return TRANSACTION_SUCCESS;

    uint8_t registerDataConfigB[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    memset(registerDataConfigB, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        Bmb_S* pBmb = &pBms->bmb[i];

        uint16_t mask = 0;
        for(int32_t j = 0; j < NUM_CELLS_PER_BMB; j++)
        {
            if(pBmb->balSwEnabled[j])
            {
                mask |= (0x0001 << j);
            }
        }
        // registerDataConfigB[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + 0] = 0x00;
        // registerDataConfigB[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + 1] = 0xF8;
        // registerDataConfigB[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + 2] = 0x7F;
        // registerDataConfigB[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + 3] = 0x00;
        registerDataConfigB[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + 4] = (uint8_t)mask;
        registerDataConfigB[(REGISTER_SIZE_BYTES * (NUM_BMBS_IN_ACCUMULATOR - i - 1)) + 5] = (uint8_t)(mask >> 8);
    }
    
    TRANSACTION_STATUS_E msgStatus;
    msgStatus = writeAll(WR_CFG_REG_B, NUM_BMBS_IN_ACCUMULATOR, registerDataConfigB);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }
    return msgStatus;


    //     for(int32_t j = 0; j < NUM_PWM_REG_B; j+=2)
    //     {
    //         if(pBmb->balSwEnabled[j+NUM_PWM_REG_A])
    //         {
    //             registerDataPwmB[(REGISTER_SIZE_BYTES * i) + (j / 2)] |= PWM_DUTY_MASK; 
    //         }
    //         if(pBmb->balSwEnabled[j+NUM_PWM_REG_A+1])
    //         {
    //             registerDataPwmB[(REGISTER_SIZE_BYTES * i) + (j / 2)] |= (((uint8_t)PWM_DUTY_MASK) << 4); 
    //         }
    //     }
    // }

    // TRANSACTION_STATUS_E msgStatus;
    // msgStatus = writeAll(WR_PWM_A, NUM_BMBS_IN_ACCUMULATOR, registerDataPwmA);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    // msgStatus = writeAll(WR_PWM_B, NUM_BMBS_IN_ACCUMULATOR, registerDataPwmB);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    // return msgStatus;
    // return TRANSACTION_SUCCESS;
    

    // uint8_t registerDataPwmA[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    // memset(registerDataPwmA, 0xFF, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // uint8_t registerDataPwmB[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    // memset(registerDataPwmB, 0xFF, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // uint8_t mask = (bmbData->minCellLocation % 2) ? (0x0F) : (0xF0);
    // if(bmbData->minCellLocation < 12)
    // {
    //     registerDataPwmA[(bmbData->minCellLocationBmb * REGISTER_SIZE_BYTES) + (bmbData->minCellLocation / 2)] = mask;
    // }
    // else
    // {
    //     registerDataPwmB[(bmbData->minCellLocationBmb * REGISTER_SIZE_BYTES) + ((bmbData->minCellLocation - 12) / 2)] = mask;
    // }

    // TRANSACTION_STATUS_E msgStatus;    

    // return TRANSACTION_SUCCESS;
    // msgStatus = writeAll(WR_PWM_A, NUM_BMBS_IN_ACCUMULATOR, registerDataPwmA);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    // msgStatus = writeAll(WR_PWM_B, NUM_BMBS_IN_ACCUMULATOR, registerDataPwmB);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    // return msgStatus;

    // return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E runSwitchPinStateMachine(BmbTaskOutputData_S* bmbData, bool balancingEnabled)
{
    static uint32_t cycleCount = 0;
    uint16_t startSadcMask = 0;
    TRANSACTION_STATUS_E msgStatus;

    if(bmbData->sPinState == SADC_REDUNDANT)
    {
        msgStatus = updateSADC(bmbData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        checkAdcMismatch(bmbData);
        cycleCount++;
        if(cycleCount >= 1)
        {
            cycleCount = 0;
            startSadcMask = ADC_CONT | ADC_OW_EVEN;
            bmbData->sPinState = SADC_OW_EVEN;
        }
        else
        {
            return TRANSACTION_SUCCESS;
        }
    }
    else if(bmbData->sPinState == SADC_OW_EVEN)
    {
        msgStatus = updateSADC(bmbData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        checkOpenWire(bmbData);
        cycleCount++;
        if(cycleCount >= 1)
        {
            cycleCount = 0;
            startSadcMask = ADC_CONT | ADC_OW_ODD;
            bmbData->sPinState = SADC_OW_ODD;
        }
        else
        {
            return TRANSACTION_SUCCESS;
        }
    }
    else if(bmbData->sPinState == SADC_OW_ODD)
    {
        msgStatus = updateSADC(bmbData);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
        checkOpenWire(bmbData);
        cycleCount++;
        if(cycleCount >= 1)
        {
            cycleCount = 0;
            if(balancingEnabled)
            {
                msgStatus = updateBalanceSwitches(bmbData);
                if(msgStatus != TRANSACTION_SUCCESS)
                {
                    return msgStatus;
                }
                msgStatus = commandAll(CMD_START_SADC | ADC_DCP, NUM_BMBS_IN_ACCUMULATOR);
                if(msgStatus != TRANSACTION_SUCCESS)
                {
                    return msgStatus;
                }
            }
            else
            {
                // Disable PWM regs here??
                startSadcMask = ADC_CONT;
            }  
            bmbData->sPinState = SW_BALANCE;
        }
        else
        {
            return TRANSACTION_SUCCESS;
        }
        
    }
    else if(bmbData->sPinState == SW_BALANCE)
    {
        if(balancingEnabled)
        {
            // msgStatus = updateBalanceSwitches(bmbData);
            // if(msgStatus != TRANSACTION_SUCCESS)
            // {
            //     return msgStatus;
            // }
            // return TRANSACTION_SUCCESS;
        }
        else
        {
            msgStatus = updateSADC(bmbData);
            if(msgStatus != TRANSACTION_SUCCESS)
            {
                return msgStatus;
            }
            checkAdcMismatch(bmbData);
        }

        cycleCount++;
        if(cycleCount >= 12)
        {
            cycleCount = 0;
            startSadcMask = ADC_CONT;
            bmbData->sPinState = SADC_REDUNDANT;

            TRANSACTION_STATUS_E msgStatus;
            uint8_t txBuffer[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
            memset(txBuffer, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
            msgStatus = writeAll(WR_CFG_REG_B, NUM_BMBS_IN_ACCUMULATOR, txBuffer);
            if(msgStatus != TRANSACTION_SUCCESS)
            {
                return msgStatus;
            }
        }
        else
        {
            return TRANSACTION_SUCCESS;
        }
    }
    else
    {
        cycleCount = 0;
        startSadcMask = ADC_CONT;
        bmbData->sPinState = SADC_REDUNDANT;
    }

    // Update SADC
    return commandAll(CMD_START_SADC | startSadcMask, NUM_BMBS_IN_ACCUMULATOR);
}

static TRANSACTION_STATUS_E updateTestData(Bmb_S* bmb)
{
    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
    // static uint8_t data[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    // static uint8_t ioSet = 0x00;
    // ioSet++;
    // data[3] = ioSet;

    // writeAll(0x0001, NUM_BMBS_IN_ACCUMULATOR, data);
    bmb[0].status = readAll(0x0022, NUM_BMBS_IN_ACCUMULATOR, registerData);
    
    for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
    {
        bmb[0].testData[i] = registerData[i];
    }
    return TRANSACTION_SUCCESS;
}

static void runBmbAlertMonitor(BmbTaskOutputData_S* bmbData)
{
    for(int32_t i = 0; i < NUM_BMB_ALERTS; i++)
    {
        bmbAlerts[i]->alertConditionPresent = bmbAlertConditionArray[i](bmbData);
        runAlertMonitor(bmbAlerts[i]);
    }

    // Accumulate alert statuses
    bool responseStatus[NUM_ALERT_RESPONSES] = { false };

    // Check each alert status
    for (uint32_t i = 0; i < NUM_BMB_ALERTS; i++)
    {
        Alert_S* alert = bmbAlerts[i];
        const AlertStatus_E alertStatus = getAlertStatus(alert);
        if ((alertStatus == ALERT_SET) || (alertStatus == ALERT_LATCHED))
        {
            // Iterate through all alert responses and set them
            for (uint32_t j = 0; j < alert->numAlertResponse; j++)
            {
                const AlertResponse_E response = alert->alertResponse[j];
                // Set the alert response to active
                responseStatus[response] = true;
            }
        }
    }
    setAmsFault(responseStatus[AMS_FAULT]);
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initBmbUpdateTask()
{
    // // TODO Remove for custom hardware where master pins tied high
    // HAL_GPIO_WritePin(MAS1_GPIO_Port, MAS1_Pin, SET);
    // HAL_GPIO_WritePin(MAS2_GPIO_Port, MAS2_Pin, SET);
    HAL_GPIO_WritePin(AMS_FAULT_OUT_GPIO_Port, AMS_FAULT_OUT_Pin, GPIO_PIN_SET);
}

void runBmbUpdateTask()
{
    // uint32_t start = HAL_GetTick();
    static bool bmbsInit = false;
    if(!bmbsInit)
    {
        wakeChain(NUM_BMBS_IN_ACCUMULATOR);
        bmbsInit = initBmbs();
        return;
    }

    // Create local data struct for bmb information
    BmbTaskOutputData_S bmbTaskOutputDataLocal;
    
    taskENTER_CRITICAL();
    bmbTaskOutputDataLocal = bmbTaskOutputData;
    taskEXIT_CRITICAL();

    wakeChain(NUM_BMBS_IN_ACCUMULATOR);
    
    TRANSACTION_STATUS_E status;

    status = updateCellVoltages(&bmbTaskOutputDataLocal);
    HANDLE_BMB_ERROR(status);

    // status = updateCellTemps(bmbTaskOutputDataLocal.bmb);
    // HANDLE_BMB_ERROR(status);

    aggregatePackData(&bmbTaskOutputDataLocal);

    // status = updateTestData(bmbTaskOutputDataLocal.bmb);
    // HANDLE_BMB_ERROR(status);

    // status = runSwitchPinStateMachine(&bmbTaskOutputDataLocal, chargerConnected);
    // HANDLE_BMB_ERROR(status);    

    runBmbAlertMonitor(&bmbTaskOutputData);

    taskENTER_CRITICAL();
    bmbTaskOutputData = bmbTaskOutputDataLocal;
    taskEXIT_CRITICAL();

    // printf("Task Time: %lu\n", HAL_GetTick()-start);

}