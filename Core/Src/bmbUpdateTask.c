/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <stdbool.h>
#include <string.h>

#include "bmbUpdateTask.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "debug.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define CMD_START_ADC           0x0360           

#define READ_VOLT_REG_A         0x0044
#define READ_VOLT_REG_B         0x0046
#define READ_VOLT_REG_C         0x0048
#define READ_VOLT_REG_D         0x004A
#define READ_VOLT_REG_E         0x0049
#define READ_VOLT_REG_F         0x004B
#define NUM_VOLT_REG            6

#define CELLS_PER_REG           3
#define CELL_REG_SIZE           REGISTER_SIZE_BYTES / CELLS_PER_REG

#define ADC_RESOLUTION_BITS     16
#define MAX_ADC_READING         0xFFFF
#define RAILED_MARGIN_BITS      2500
#define ADC_RESOLUTION          0.00015f
#define ADC_OFFSET_VOLT         1.5f

#define DIE_TEMP_RESOLUTION     0.02f
#define DIE_TEMP_OFFSET_DEGC    -73.0f

#define TEST_REG                0x002C

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

uint16_t readVoltReg[NUM_VOLT_REG] =
{
    READ_VOLT_REG_A, READ_VOLT_REG_B,
    READ_VOLT_REG_C, READ_VOLT_REG_D,
    READ_VOLT_REG_E, READ_VOLT_REG_F,
};

bool bmbsInit = false;

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static bool initBmbs();
static TRANSACTION_STATUS_E updateBmbTelemetry(Bmb_S* bmb);
static TRANSACTION_STATUS_E updateTestData(Bmb_S* bmb);

/* ==================================================================== */
/* ============================== MACROS ============================== */
/* ==================================================================== */

#define HANDLE_BMB_ERROR(error) \
    if(error != TRANSACTION_SUCCESS) { \
        if(error == TRANSACTION_CRC_ERROR) \
        { \
            Debug("Chain break!\n"); \
            return; \
        } \
        else if(error == TRANSACTION_SPI_ERROR) \
        { \
            Debug("SPI Failure, reseting micro...\n"); \
            HAL_NVIC_SystemReset(); \
        } \
        else if(error == TRANSACTION_POR_ERROR) \
        { \
            Debug("Power reset detected, reinitializing...\n"); \
            bmbsInit = initBmbs(); \
            return; \
        } \
    }

// Check if 16bit ADC is railed
#define IS_ADC_RAILED(rawAdc) ((rawAdc < RAILED_MARGIN_BITS) || (rawAdc > (MAX_ADC_READING - RAILED_MARGIN_BITS)))

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

    uint8_t data[6] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00};
    initSuccess |= (writeAll(0x0001, NUM_BMBS_IN_ACCUMULATOR, data) != TRANSACTION_SUCCESS);
    initSuccess |= (commandAll(CMD_START_ADC, NUM_BMBS_IN_ACCUMULATOR) != TRANSACTION_SUCCESS);

    if(!initSuccess)
    {
        Debug("BMBs failed to initialize!\n");
        return false;
    }
    return true;
}

static TRANSACTION_STATUS_E updateBmbTelemetry(Bmb_S* bmb)
{
    TRANSACTION_STATUS_E msgStatus;
    uint8_t registerData[REGISTER_SIZE_BYTES];

    for(int32_t i = 0; i < NUM_VOLT_REG-1; i++)
    {
        memset(registerData, 0, REGISTER_SIZE_BYTES);
        msgStatus = readAll(readVoltReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
        if((msgStatus != TRANSACTION_SUCCESS) && (msgStatus != TRANSACTION_CRC_ERROR) && (msgStatus != TRANSACTION_COMMAND_COUNTER_ERROR))
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
                if(IS_ADC_RAILED(rawAdc))
                {
                    bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = BAD;
                }
                else
                {
                    bmb[k].cellVoltage[(i * CELLS_PER_REG) + j] = CONVERT_16_BIT_ADC(rawAdc);
                    bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = GOOD;
                }
            }
        }
    }

    memset(registerData, 0, REGISTER_SIZE_BYTES);
    msgStatus = readAll(readVoltReg[5], NUM_BMBS_IN_ACCUMULATOR, registerData);
    if((msgStatus != TRANSACTION_SUCCESS) && (msgStatus != TRANSACTION_CRC_ERROR) && (msgStatus != TRANSACTION_COMMAND_COUNTER_ERROR))
    {
        return msgStatus;
    }
    for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    {
        uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES)];
        uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + 1];
        uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
        if(IS_ADC_RAILED(rawAdc))
        {
            bmb[k].cellVoltageStatus[(5 * CELLS_PER_REG)] = BAD;
        }
        else
        {
            bmb[k].cellVoltage[(5 * CELLS_PER_REG)] = CONVERT_16_BIT_ADC(rawAdc);
            bmb[k].cellVoltageStatus[(5 * CELLS_PER_REG)] = GOOD;
        }
    }

    msgStatus = commandAll(CMD_START_ADC, NUM_BMBS_IN_ACCUMULATOR);
    if((msgStatus != TRANSACTION_SUCCESS) && (msgStatus != TRANSACTION_CRC_ERROR))
    {
        return msgStatus;
    }
    return msgStatus;
}

static TRANSACTION_STATUS_E updateTestData(Bmb_S* bmb)
{
    uint8_t registerData[REGISTER_SIZE_BYTES];
    memset(registerData, 0, REGISTER_SIZE_BYTES);
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

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initBmbUpdateTask()
{
    // TODO Remove for custom hardware where master pins tied high
    HAL_GPIO_WritePin(MAS1_GPIO_Port, MAS1_Pin, SET);
    HAL_GPIO_WritePin(MAS2_GPIO_Port, MAS2_Pin, SET);
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

    wakeChain(NUM_BMBS_IN_ACCUMULATOR);
    HANDLE_BMB_ERROR(updateBmbTelemetry(bmbTaskOutputDataLocal.bmb));
    HANDLE_BMB_ERROR(updateTestData(bmbTaskOutputDataLocal.bmb));

    taskENTER_CRITICAL();
    bmbTaskOutputData = bmbTaskOutputDataLocal;
    taskEXIT_CRITICAL();

}