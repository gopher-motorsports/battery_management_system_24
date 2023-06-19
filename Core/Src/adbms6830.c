/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "stdint.h"
#include "main.h"

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static spiTxRx(uint8_t *txBuff, uint8_t *rxBuff, uint32_t size);
static uint16_t calcPec(uint8_t *packet, uint32_t numBytes);

static pollBmbs();
static uint8_t writeBmbs(uint16_t command, uint8_t *txBuff, uint32_t numBmbs);
static uint8_t readBmbs(uint16_t command, uint8_t *rxBuff, uint32_t numBmbs);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static uint16_t calcPec(uint8_t *packet, uint32_t numBytes)
{
    uint16_t pec = 0x0020;
    for(int32_t i = 0; i < numBytes; i++)
    {
        for(int32_t j = 7; j >= 0; j--)
        {
            uint16_t base = ((pec >> 15) ^ ((packet[i] >> j) & 0x01));
            if(base == 0x01)
            {
                pec ^= (base << 14) | (base << 10) | (base << 8) | (base << 7) | (base << 4) | (base << 3);
                pec |= base;
            }
            pec <<= 1;
        } 
    }
    return pec;
}

static uint8_t writeBmbs(uint16_t command, uint8_t *txBuff, uint32_t numBmbs)
{
    
}

static uint8_t readBmbs(uint16_t command, uint8_t *rxBuff, uint32_t numBmbs)
{

}


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

