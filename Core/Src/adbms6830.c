/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "stdint.h"
#include "string.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define BITS_IN_BYTE        8
#define BITS_IN_WORD        16
#define BYTES_IN_WORD       2

#define CRC_INITIAL         0x0020
#define CRC_POLYNOMIAL      0x4599

#define CRC_SIZE_BYTES           2
#define COMMAND_SIZE_BYTES       2
#define REGISTER_SIZE_BYTES      6

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static spiTxRx(uint8_t *txBuff, uint8_t *rxBuff, uint32_t size);
static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes);

static pollAll();
static uint8_t writeAll(uint16_t command, uint8_t *txData, uint32_t numBmbs);
static uint8_t readAll(uint16_t command, uint8_t *rxData, uint32_t numBmbs);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes)
{
    uint16_t crc = CRC_INITIAL;
    for(int32_t i = 0; i < numBytes; i++)
    {
        for(int32_t j = (BITS_IN_BYTE - 1); j >= 0; j--)
        {
            if((crc >> (BITS_IN_WORD - 1)) ^ ((packet[i] >> j) & 0x01))
            {
                crc ^= CRC_POLYNOMIAL;
            }
            crc <<= 1;
        } 
    }
    return crc;
}

static uint8_t writeAll(uint16_t command, uint8_t *txData, uint32_t numBmbs)
{
    // Create a transmit message buffer
    // Size in bytes: Command Word(2) + Command CRC(2) + [Register data(6) + Data CRC(2)] * numBmbs
    uint8_t txBuffer[COMMAND_SIZE_BYTES + CRC_SIZE_BYTES + (numBmbs * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES))]; // Should NUMBMBS be known gloabally?
    
    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    // Calculate the CRC on the register data packet (2 byte CRC on 6 byte packet)
    uint16_t dataCRC = calcCrc(txData, REGISTER_SIZE_BYTES);

    // For each bmb, append a copy of the register data and corresponding CRC to the tx buffer  
    for(int32_t i = 0; i < numBmbs; i++)
    {
        for(int32_t j = 0; j < REGISTER_SIZE_BYTES; j++)
        {
            txBuffer[(COMMAND_SIZE_BYTES + CRC_SIZE_BYTES) + (i * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)) + j] = txData[j];
        }
        txBuffer[(COMMAND_SIZE_BYTES + CRC_SIZE_BYTES) + (i * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)) + REGISTER_SIZE_BYTES] = (uint8_t)(dataCRC >> BITS_IN_BYTE);;
        txBuffer[(COMMAND_SIZE_BYTES + CRC_SIZE_BYTES) + (i * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)) + REGISTER_SIZE_BYTES + 1] = (uint8_t)(dataCRC);;
    }

    // --SPI--
}

static uint8_t readAll(uint16_t command, uint8_t *rxData, uint32_t numBmbs)
{
    // Create transmit and recieve message buffers
    // Size in bytes: Command Word(2) + Command CRC(2) + [Register data(6) + Data CRC(2)] * numBmbs
    // Tx Register data is not populated on a read command, but is sent as dummy data while data is read across spi
    uint8_t txBuffer[COMMAND_SIZE_BYTES + CRC_SIZE_BYTES + (numBmbs * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES))];
    uint8_t rxBuffer[COMMAND_SIZE_BYTES + CRC_SIZE_BYTES + (numBmbs * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES))];
    
    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    // --SPI--

    for(int32_t i = 0; i < numBmbs; i++)
    {
        uint8_t registerData[REGISTER_SIZE_BYTES];
        for(int32_t j = 0; j < REGISTER_SIZE_BYTES; j++)
        {
            registerData[j] = rxBuffer[(COMMAND_SIZE_BYTES + CRC_SIZE_BYTES) + (i * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)) + j];
        }

        uint16_t registerCRC0 = rxBuffer[(COMMAND_SIZE_BYTES + CRC_SIZE_BYTES) + (i * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)) + REGISTER_SIZE_BYTES];
        uint16_t registerCRC1 = rxBuffer[(COMMAND_SIZE_BYTES + CRC_SIZE_BYTES) + (i * (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)) + REGISTER_SIZE_BYTES + 1];
        uint16_t registerCRC = (registerCRC0 << BITS_IN_BYTE) | (registerCRC1);

        if(calcCrc(registerData, REGISTER_SIZE_BYTES) == registerCRC)
        {
            memcpy(rxData + (i * REGISTER_SIZE_BYTES), registerData, REGISTER_SIZE_BYTES); 
        }
    }
}


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

