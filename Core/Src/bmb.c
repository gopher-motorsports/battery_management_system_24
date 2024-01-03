/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdbool.h>
#include "bmb.h"
#include "adbms6830.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define CMD_START_ADC       0x0360           

#define READ_VOLT_REG_A     0x0004
#define READ_VOLT_REG_B     0x0006
#define READ_VOLT_REG_C     0x0008
#define READ_VOLT_REG_D     0x000A
#define READ_VOLT_REG_E     0x0009
#define READ_VOLT_REG_F     0x000B
#define NUM_VOLT_REG        6

#define CELLS_PER_REG       3
#define CELL_REG_SIZE       REGISTER_SIZE_BYTES / CELLS_PER_REG

#define ADC_RESOLUTION_BITS     16
#define MAX_ADC_READING         0xFFFF
#define VOLTS_PER_BIT           0.0001f

#define RAILED_MARGIN_BITS  2500

#define TEST_REG    0x002C

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

// uint16_t readVoltReg[NUM_VOLT_REG] =
// {
//     READ_VOLT_REG_A, READ_VOLT_REG_B,
//     READ_VOLT_REG_C, READ_VOLT_REG_D,
//     READ_VOLT_REG_E, READ_VOLT_REG_F,
// };

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

// static bool isAdcRailed(uint16_t rawAdc);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

/*!
  @brief   Check if a 14-bit ADC sensor value is railed (near minimum or maximum).
  @param   rawAdcVal - The raw 14-bit ADC sensor value to check.
  @return  True if the sensor value is within the margin of railed values, false otherwise.
*/
// static bool isAdcRailed(uint16_t rawAdc)
// {
// 	// Determine if the adc reading is within a margin of the railed values
//     return ((rawAdc < RAILED_MARGIN_BITS) || (rawAdc > (MAX_ADC_READING - RAILED_MARGIN_BITS)));
// }

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

// void updateBmbTelemetry(Bmb_S* bmb, uint32_t numBmbs)
// {
//     // TODO add polling to check scan status - prevent initialization error 

//     for(int32_t i = 0; i < NUM_VOLT_REG; i++)
//     {
//         uint8_t registerData[REGISTER_SIZE_BYTES];
//         readAll(readVoltReg[i], registerData, numBmbs);
//         for(int32_t j = 0; j < CELLS_PER_REG; j++)
//         {
//             for(int32_t k = 0; k < numBmbs; k++)
//             {
//                 uint16_t rawAdcLSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE)];
//                 uint16_t rawAdcMSB = registerData[(k * REGISTER_SIZE_BYTES) + (j * CELL_REG_SIZE) + 1];
//                 uint16_t rawAdc = (rawAdcMSB << BITS_IN_BYTE) | (rawAdcLSB);
//                 if(isAdcRailed(rawAdc))
//                 {
//                     bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = BAD;
//                 }
//                 else
//                 {
//                     bmb[k].cellVoltage[(i * CELLS_PER_REG) + j] = rawAdc * VOLTS_PER_BIT;
//                     bmb[k].cellVoltageStatus[(i * CELLS_PER_REG) + j] = GOOD;
//                 }
//             }
//         }
//     }

//     commandAll(CMD_START_ADC, numBmbs);
// }

void testRead(Bmb_S* bmb, uint32_t numBmbs)
{
    uint8_t registerData[REGISTER_SIZE_BYTES];
    // uint8_t data[6] = {0x01, 0x00, 0x00, 0x05, 0x01, 0x00};
    readAll(0x0002, registerData, numBmbs);
    // writeAll(0x0001, data, numBmbs);
    
    for(int32_t i = 0; i < REGISTER_SIZE_BYTES; i++)
    {
        // bmb[0].testData[i] = 0x02;
        bmb[0].testData[i] = registerData[i];
    }
}