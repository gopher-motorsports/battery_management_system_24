/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <stdbool.h>
#include <string.h>

#include "bmbUpdateTask.h"
#include "main.h"
#include "cmsis_os.h"
#include "lookupTable.h"
#include <stdio.h>
#include "debug.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define CMD_START_CADC          0x0360
#define CMD_START_AUX_ADC       0x0410  

#define WR_CFG_REG_A            0x0001

#define READ_VOLT_REG_A         0x0044
#define READ_VOLT_REG_B         0x0046
#define READ_VOLT_REG_C         0x0048
#define READ_VOLT_REG_D         0x004A
#define READ_VOLT_REG_E         0x0049
#define READ_VOLT_REG_F         0x004B
#define NUM_VOLT_REG            6

#define READ_AUX_REG_A          0x0019
#define READ_AUX_REG_B          0x001A
#define READ_AUX_REG_C          0x001B
#define NUM_AUX_REG             3

#define WR_PWM_A                0x0020
#define WR_PWM_B                0x0022

#define TEMPS_PER_MUX           8
#define CELLS_PER_REG           3
#define CELL_REG_SIZE           REGISTER_SIZE_BYTES / CELLS_PER_REG

#define ADC_RESOLUTION          0.00015f
#define ADC_OFFSET_VOLT         1.5f

#define DIE_TEMP_RESOLUTION     0.02f
#define DIE_TEMP_OFFSET_DEGC    -73.0f

#define TEST_REG                0x002C

// The number of values contained in a lookup table
#define LUT_SIZE 33

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

uint16_t readVoltReg[NUM_VOLT_REG] =
{
    READ_VOLT_REG_A, READ_VOLT_REG_B,
    READ_VOLT_REG_C, READ_VOLT_REG_D,
    READ_VOLT_REG_E, READ_VOLT_REG_F,
};

uint16_t readAuxReg[NUM_AUX_REG] =
{
    READ_AUX_REG_A,
    READ_AUX_REG_B,
    READ_AUX_REG_C
};

bool bmbsInit = false;

const float temperatureArray[LUT_SIZE] =
			{120, 115, 110, 105, 100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50, 45, 40, 35,
			 30, 25, 20, 15, 10, 5, 0, -5, -10, -15, -20, -25, -30, -35, -40};

const float cellTempVoltageArray[LUT_SIZE] =
			{0.1741318359, 0.193533737, 0.2155192602, 0.2404628241, 0.2687907644, 0.3009857595, 0.3375901116, 0.3792069573, 0.4264981201, 0.480176881, 0.5409934732, 0.6097106855, 0.6870667304, 0.7737227417, 0.870193244, 0.976760075, 1.093373849, 1.219552143, 1.354289544, 1.496, 1.642514054, 1.791149899, 1.938865924, 2.082484747, 2.218959086, 2.345635446, 2.460468742, 2.562151894, 2.650145243, 2.724613713, 2.786297043, 2.836345972, 2.87615517};

const float boardTempVoltageArray[LUT_SIZE] =
			{0.1741318359, 0.193533737, 0.2155192602, 0.2404628241, 0.2687907644, 0.3009857595, 0.3375901116, 0.3792069573, 0.4264981201, 0.480176881, 0.5409934732, 0.6097106855, 0.6870667304, 0.7737227417, 0.870193244, 0.976760075, 1.093373849, 1.219552143, 1.354289544, 1.496, 1.642514054, 1.791149899, 1.938865924, 2.082484747, 2.218959086, 2.345635446, 2.460468742, 2.562151894, 2.650145243, 2.724613713, 2.786297043, 2.836345972, 2.87615517};

LookupTable_S cellTempTable =  { .length = LUT_SIZE, .x = cellTempVoltageArray, .y = temperatureArray};
LookupTable_S boardTempTable = { .length = LUT_SIZE, .x = boardTempVoltageArray, .y = temperatureArray};

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static bool initBmbs();
static TRANSACTION_STATUS_E updateCellVoltages(Bmb_S* bmb);
static TRANSACTION_STATUS_E updateCellTemps(Bmb_S* bmb);
static TRANSACTION_STATUS_E updateTestData(Bmb_S* bmb);
static TRANSACTION_STATUS_E balanceAll(Bmb_S* bmb);

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
            return; \
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
    initSuccess |= (commandAll(CMD_START_CADC, NUM_BMBS_IN_ACCUMULATOR) != TRANSACTION_SUCCESS);
    initSuccess |= (commandAll(CMD_START_AUX_ADC, NUM_BMBS_IN_ACCUMULATOR) != TRANSACTION_SUCCESS);

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
    static MUX_STATE_E muxState = MUX_STATE_0;


    // Create a register buffer for read transactions
    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    TRANSACTION_STATUS_E msgStatus;

    for(int32_t i = 0; i < NUM_AUX_REG-1; i++)
    {
        // Clear register buffer
        memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

        // Read aux register from all BMBs
        msgStatus = readAll(readAuxReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
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

                // Convert ADC to temp and populate bmb struct 
                bmb[k].cellTemp[(muxState) + 2 * ((i * CELLS_PER_REG) + j)] = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
                bmb[k].cellTempStatus[(muxState) + 2 * ((i * CELLS_PER_REG) + j)] = GOOD;
            }
        }
    }

    // Clear register buffer
    memset(registerData, 0, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    // Read aux register from all BMBs
    msgStatus = readAll(readAuxReg[2], NUM_BMBS_IN_ACCUMULATOR, registerData);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }

    // Sort through aux register and populate BMB struct cell temps
    for(int32_t j = 0; j < CELLS_PER_REG-1; j++)
    {
        for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
        {
            // Extract 16bit ADC from register
            uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
            uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
            uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);

            // Convert ADC to temp and populate bmb struct 
            bmb[k].cellTemp[(muxState) + 2 * ((2 * CELLS_PER_REG) + j)] = lookup(CONVERT_16_BIT_ADC(rawAdc), &cellTempTable);
            bmb[k].cellTempStatus[(muxState) + 2 * ((2 * CELLS_PER_REG) + j)] = GOOD;
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
        bmb[k].boardTemp = lookup(CONVERT_16_BIT_ADC(rawAdc), &boardTempTable);
        bmb[k].boardTempStatus = GOOD;
    }

    // Cycle mux state
    muxState++;
    muxState %= NUM_MUX_STATES;

    uint8_t data[6] = {0x01, 0x00, 0x00, 0xFF, 0x00, 0x00};
    data[4] = ((uint8_t)muxState << 1) | (0x01);
    msgStatus = writeAll(WR_CFG_REG_A, NUM_BMBS_IN_ACCUMULATOR, data);
    if((msgStatus != TRANSACTION_SUCCESS) && (msgStatus != TRANSACTION_CRC_ERROR) && (msgStatus != TRANSACTION_WRITE_REJECT) && (msgStatus != TRANSACTION_COMMAND_COUNTER_ERROR))
    {
        return msgStatus;
    }

    // Start Aux ADC conversion
    msgStatus = commandAll(CMD_START_AUX_ADC, NUM_BMBS_IN_ACCUMULATOR);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E updateCellVoltages(Bmb_S* bmb)
{
    TRANSACTION_STATUS_E msgStatus;
    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];

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
                bmb[k].cellVoltage[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = GOOD;
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
        bmb[k].cellVoltage[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
        bmb[k].cellVoltageStatus[(5 * CELLS_PER_REG)] = GOOD;
    }

    msgStatus = commandAll(CMD_START_CADC, NUM_BMBS_IN_ACCUMULATOR);
    if(msgStatus != TRANSACTION_SUCCESS)
    {
        return msgStatus;
    }
    return msgStatus;
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
    bmb[0].status = readAll(0x0002, NUM_BMBS_IN_ACCUMULATOR, registerData);
    
    for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
    {
        bmb[0].testData[i] = registerData[i];
    }
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E balanceAll(Bmb_S* bmb)
{
    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    memset(registerData, 0xFF, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);

    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        writeAll(WR_PWM_A, NUM_BMBS_IN_ACCUMULATOR, registerData);
        writeAll(WR_PWM_B, NUM_BMBS_IN_ACCUMULATOR, registerData);
    }
    return TRANSACTION_SUCCESS;
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initBmbUpdateTask()
{
    // // TODO Remove for custom hardware where master pins tied high
    // HAL_GPIO_WritePin(MAS1_GPIO_Port, MAS1_Pin, SET);
    // HAL_GPIO_WritePin(MAS2_GPIO_Port, MAS2_Pin, SET);
}

void runBmbUpdateTask()
{
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
    status = updateCellVoltages(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    status = updateCellTemps(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    status = updateTestData(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    status = balanceAll(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    taskENTER_CRITICAL();
    bmbTaskOutputData = bmbTaskOutputDataLocal;
    taskEXIT_CRITICAL();

}