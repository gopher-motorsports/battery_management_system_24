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

#define WR_PWM_A                0x0020
#define WR_PWM_B                0x0021

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
static TRANSACTION_STATUS_E updateCellDiagnostics(BmbTaskOutputData_S* bmbData);
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
    initSuccess &= (commandAll((CMD_START_CADC | ADC_RD | ADC_CONT), NUM_BMBS_IN_ACCUMULATOR) == TRANSACTION_SUCCESS);
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
                bmb[k].cellVoltageAvg[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                bmb[k].cellVoltageAvgStatus[(i * CELLS_PER_REG) + j] = GOOD;
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
        bmb[k].cellVoltageAvg[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
        bmb[k].cellVoltageAvgStatus[(5 * CELLS_PER_REG)] = GOOD;
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
                bmb[k].cellVoltageFiltered[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                bmb[k].cellVoltageFilteredStatus[(i * CELLS_PER_REG) + j] = GOOD;
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
        bmb[k].cellVoltageFiltered[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
        bmb[k].cellVoltageFilteredStatus[(5 * CELLS_PER_REG)] = GOOD;
    }

    // msgStatus = commandAll(CMD_START_CADC | ADC_CONT | ADC_RD, NUM_BMBS_IN_ACCUMULATOR);
    // if(msgStatus != TRANSACTION_SUCCESS)
    // {
    //     return msgStatus;
    // }
    return msgStatus;
}

static TRANSACTION_STATUS_E updateCellDiagnostics(BmbTaskOutputData_S* bmbData)
{
    // SADC diagnostic state
    static SADC_STATE_E sadcDiagnosticState = SADC_REDUNDANT;

    TRANSACTION_STATUS_E msgStatus = TRANSACTION_SUCCESS;

    bmbData->sadcState = sadcDiagnosticState;
    if(sadcDiagnosticState < SADC_PAUSE_1)
    {
        uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
        static uint16_t openWireMask = 0;

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

                    if(sadcDiagnosticState == SADC_REDUNDANT)
                    {
                        if(fabs(bmbData->bmb[k].cellVoltage[cellIndex] - bmbData->bmb[k].cellVoltageRedundant[cellIndex]) > 0.2f)
                        {
                            bmbData->bmb[k].adcMismatch[cellIndex] = true;
                        }
                        else
                        {
                            bmbData->bmb[k].adcMismatch[cellIndex] = false;
                        }
                    }
                    else if((sadcDiagnosticState == SADC_OW_EVEN) && (bool)(cellIndex % 2))
                    {
                        if(bmbData->bmb[k].cellVoltageRedundant[cellIndex] < 0.5f)
                        {
                            openWireMask |= ((uint16_t)1<<cellIndex);
                        }
                        else
                        {
                            openWireMask &= ~((uint16_t)1<<cellIndex);
                        }
                    }
                    else if((sadcDiagnosticState == SADC_OW_ODD) && !(bool)(cellIndex % 2))
                    {
                        if(bmbData->bmb[k].cellVoltageRedundant[cellIndex] < 0.5f)
                        {
                            openWireMask |= ((uint16_t)1<<cellIndex);
                        }
                        else
                        {
                            openWireMask &= ~((uint16_t)1<<cellIndex);
                        }
                    }
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

            if(sadcDiagnosticState == SADC_REDUNDANT)
            {
                if(fabs(bmbData->bmb[k].cellVoltage[cellIndex] - bmbData->bmb[k].cellVoltageRedundant[cellIndex]) > 0.2f)
                {
                    bmbData->bmb[k].adcMismatch[cellIndex] = true;
                }
                else
                {
                    bmbData->bmb[k].adcMismatch[cellIndex] = false;
                }
            }
            else if((sadcDiagnosticState == SADC_OW_EVEN) && (bool)(cellIndex % 2))
            {
                if(bmbData->bmb[k].cellVoltageRedundant[cellIndex] < 0.5f)
                {
                    openWireMask |= ((uint16_t)1<<cellIndex);
                }
                else
                {
                    openWireMask &= ~((uint16_t)1<<cellIndex);
                }
            }
            else if((sadcDiagnosticState == SADC_OW_ODD) && !(bool)(cellIndex % 2))
            {
                if(bmbData->bmb[k].cellVoltageRedundant[cellIndex] < 0.5f)
                {
                    openWireMask |= ((uint16_t)1<<cellIndex);
                }
                else
                {
                    openWireMask &= ~((uint16_t)1<<cellIndex);
                }
            }
        }

        //Check for Open wires
        if((sadcDiagnosticState == SADC_OW_EVEN) || (sadcDiagnosticState == SADC_OW_ODD))
        {
            for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
            {
                for(int32_t j = 1; j < NUM_CELLS_PER_BMB; j++)
                {
                    bmbData->bmb[i].openWire[j] = (((openWireMask >> (j-1)) & 0x0003) == 0x0003);
                }  
                bmbData->bmb[i].openWire[0] = (((openWireMask) & 0x0003) == 0x0001);
                bmbData->bmb[i].openWire[NUM_CELLS_PER_BMB] = (((openWireMask) & 0xC000) == 0x8000); 
            }
        }
    }

    // Cycle SADC diagnostic state
    sadcDiagnosticState++;
    sadcDiagnosticState %= NUM_SADC_STATES;

    if(sadcDiagnosticState == SADC_REDUNDANT)
    {
        msgStatus = commandAll(CMD_START_SADC | ADC_CONT, NUM_BMBS_IN_ACCUMULATOR);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
    }
    else if(sadcDiagnosticState == SADC_OW_EVEN)
    {
        msgStatus = commandAll(CMD_START_SADC | ADC_CONT | ADC_OW_EVEN, NUM_BMBS_IN_ACCUMULATOR);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
    }
    else if(sadcDiagnosticState == SADC_OW_ODD)
    {
        msgStatus = commandAll(CMD_START_SADC | ADC_CONT | ADC_OW_ODD, NUM_BMBS_IN_ACCUMULATOR);
        if(msgStatus != TRANSACTION_SUCCESS)
        {
            return msgStatus;
        }
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
    bmb[0].status = readAll(0x0022, NUM_BMBS_IN_ACCUMULATOR, registerData);
    
    for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
    {
        bmb[0].testData[i] = registerData[i];
    }
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E balanceAll(Bmb_S* bmb)
{
    uint8_t registerData[REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR];
    memset(registerData, 0x00, REGISTER_SIZE_BYTES * NUM_BMBS_IN_ACCUMULATOR);
    registerData[4] = 0xFF;
    registerData[5] = 0xFF;

    for(int32_t i = 0; i < NUM_BMBS_IN_ACCUMULATOR; i++)
    {
        // writeAll(WR_PWM_A, NUM_BMBS_IN_ACCUMULATOR, registerData);
        // writeAll(WR_PWM_B, NUM_BMBS_IN_ACCUMULATOR, registerData);
        writeAll(0x0024, NUM_BMBS_IN_ACCUMULATOR, registerData);

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
    status = updateCellVoltages(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    status = updateCellDiagnostics(&bmbTaskOutputDataLocal);
    HANDLE_BMB_ERROR(status);

    status = updateCellTemps(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    status = updateTestData(bmbTaskOutputDataLocal.bmb);
    HANDLE_BMB_ERROR(status);

    // status = balanceAll(bmbTaskOutputDataLocal.bmb);
    // HANDLE_BMB_ERROR(status);  

    taskENTER_CRITICAL();
    bmbTaskOutputData = bmbTaskOutputDataLocal;
    taskEXIT_CRITICAL();

    // printf("Task Time: %lu\n", HAL_GetTick()-start);

}