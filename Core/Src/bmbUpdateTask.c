/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <stdbool.h>
#include <string.h>

#include "bmbUpdateTask.h"
#include "main.h"
#include "cmsis_os.h"

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
#define ADC_RESOLUTION          0.00015f
#define ADC_OFFSET              1.5f

#define RAILED_MARGIN_BITS      2500

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

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static bool isAdcRailed(uint16_t rawAdc);
static void updateBmbTelemetry(Bmb_S* bmb);
static void updateTestData(Bmb_S* bmb);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

/*!
  @brief   Check if a 14-bit ADC sensor value is railed (near minimum or maximum).
  @param   rawAdcVal - The raw 14-bit ADC sensor value to check.
  @return  True if the sensor value is within the margin of railed values, false otherwise.
*/
static bool isAdcRailed(uint16_t rawAdc)
{
	// Determine if the adc reading is within a margin of the railed values
    return ((rawAdc < RAILED_MARGIN_BITS) || (rawAdc > (MAX_ADC_READING - RAILED_MARGIN_BITS)));
}

static void updateBmbTelemetry(Bmb_S* bmb)
{
    // TODO add polling to check scan status - prevent initialization error 

    for(int32_t i = 0; i < NUM_VOLT_REG-1; i++)
    {
        uint8_t registerData[REGISTER_SIZE_BYTES];
        memset(registerData, 0, REGISTER_SIZE_BYTES);
        readAll(readVoltReg[i], NUM_BMBS_IN_ACCUMULATOR, registerData);
        for(int32_t j = 0; j < CELLS_PER_REG; j++)
        {
            for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
            {
                uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
                uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
                uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
                if(isAdcRailed(rawAdc))
                {
                    bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = BAD;
                }
                else
                {
                    bmb[k].cellVoltage[(i * CELLS_PER_REG) + j] = (rawAdc * ADC_RESOLUTION) + ADC_OFFSET;
                    bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = GOOD;
                }
            }
        }
    }

    uint8_t registerData[REGISTER_SIZE_BYTES];
    memset(registerData, 0, REGISTER_SIZE_BYTES);
    readAll(readVoltReg[5], NUM_BMBS_IN_ACCUMULATOR, registerData);
    for(int32_t k = 0; k < NUM_BMBS_IN_ACCUMULATOR; k++)
    {
        uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES)];
        uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + 1];
        uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
        if(isAdcRailed(rawAdc))
        {
            bmb[k].cellVoltageStatus[(5 * CELLS_PER_REG)] = BAD;
        }
        else
        {
            bmb[k].cellVoltage[(5 * CELLS_PER_REG)] = (rawAdc * ADC_RESOLUTION) + ADC_OFFSET;
            bmb[k].cellVoltageStatus[(5 * CELLS_PER_REG)] = GOOD;
        }
    }

    commandAll(CMD_START_ADC, NUM_BMBS_IN_ACCUMULATOR);
}

static void updateTestData(Bmb_S* bmb)
{
    uint8_t registerData[REGISTER_SIZE_BYTES];
    memset(registerData, 0, REGISTER_SIZE_BYTES);
    static uint8_t data[6] = {0x01, 0x00, 0x00, 0x03, 0x01, 0x00};
    static uint8_t ioSet = 0x00;
    ioSet++;
    data[4] = ioSet;

    writeAll(0x0001, NUM_BMBS_IN_ACCUMULATOR, data);
    bmb[0].status = readAll(0x0002, NUM_BMBS_IN_ACCUMULATOR, registerData);
    
    for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
    {
        bmb[0].testData[i] = registerData[i];
    }
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

void initBmbUpdateTask()
{
    HAL_GPIO_WritePin(MAS1_GPIO_Port, MAS1_Pin, SET);
    HAL_GPIO_WritePin(MAS2_GPIO_Port, MAS2_Pin, SET);

    wakeChain(NUM_BMBS_IN_ACCUMULATOR);
    commandAll(CMD_START_ADC, NUM_BMBS_IN_ACCUMULATOR);
}

void runBmbUpdateTask()
{
    BmbTaskOutputData_S bmbTaskOutputDataLocal;

    wakeChain(NUM_BMBS_IN_ACCUMULATOR);
    updateBmbTelemetry(bmbTaskOutputDataLocal.bmb);
    updateTestData(bmbTaskOutputDataLocal.bmb);

    taskENTER_CRITICAL();
    bmbTaskOutputData = bmbTaskOutputDataLocal;
    taskEXIT_CRITICAL();

}