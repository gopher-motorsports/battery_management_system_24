/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <string.h>
#include <stdbool.h>
#include "adbms6830.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define BITS_IN_BYTE        8
#define BITS_IN_WORD        16
#define BYTES_IN_WORD       2

#define CRC_INITIAL         0x0020
#define CRC_POLYNOMIAL      0x4599
#define CRC_LUT_SIZE        256

#define CRC_SIZE_BYTES           2
#define COMMAND_SIZE_BYTES       2
#define REGISTER_SIZE_BYTES      6

#define COMMAND_PACKET_LENGTH    (COMMAND_SIZE_BYTES + CRC_SIZE_BYTES)
#define REGISTER_PACKET_LENGTH   (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)

#define TRANSACTION_ATTEMPTS    3

#define TRANSACTIONS_BEFORE_RETRY   100

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    PORTA = 0,
    PORTB
} PORT_E;

typedef enum
{
    LOST_COMMS = 0,
    UNIDIRECTIONAL_COMMS,
    BIDIRECTIONAL_COMMS
} COMMS_STATUS_E;

/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */

typedef struct
{
    COMMS_STATUS_E commsStatus;
    uint8_t portABmbs;
    uint8_t portBBmbs;
    uint32_t numUniTransactions;
} CHAIN_STATUS_S;

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

CHAIN_STATUS_S chainStatus = {.commsStatus = LOST_COMMS, .portABmbs = 0, .portBBmbs = 0, .numUniTransactions = 0};

uint16_t crcTable[CRC_LUT_SIZE] = 
{
    0x0000, 0xC599, 0xCEAB, 0x0B32, 0xD8CF, 0x1D56, 0x1664, 0xD3FD, 0xF407, 0x319E, 0x3AAC, 0xFF35, 0x2CC8, 0xE951, 0xE263, 0x27FA, 
    0xAD97, 0x680E, 0x633C, 0xA6A5, 0x7558, 0xB0C1, 0xBBF3, 0x7E6A, 0x5990, 0x9C09, 0x973B, 0x52A2, 0x815F, 0x44C6, 0x4FF4, 0x8A6D, 
    0x5B2E, 0x9EB7, 0x9585, 0x501C, 0x83E1, 0x4678, 0x4D4A, 0x88D3, 0xAF29, 0x6AB0, 0x6182, 0xA41B, 0x77E6, 0xB27F, 0xB94D, 0x7CD4, 
    0xF6B9, 0x3320, 0x3812, 0xFD8B, 0x2E76, 0xEBEF, 0xE0DD, 0x2544, 0x02BE, 0xC727, 0xCC15, 0x098C, 0xDA71, 0x1FE8, 0x14DA, 0xD143, 
    0xF3C5, 0x365C, 0x3D6E, 0xF8F7, 0x2B0A, 0xEE93, 0xE5A1, 0x2038, 0x07C2, 0xC25B, 0xC969, 0x0CF0, 0xDF0D, 0x1A94, 0x11A6, 0xD43F, 
    0x5E52, 0x9BCB, 0x90F9, 0x5560, 0x869D, 0x4304, 0x4836, 0x8DAF, 0xAA55, 0x6FCC, 0x64FE, 0xA167, 0x729A, 0xB703, 0xBC31, 0x79A8, 
    0xA8EB, 0x6D72, 0x6640, 0xA3D9, 0x7024, 0xB5BD, 0xBE8F, 0x7B16, 0x5CEC, 0x9975, 0x9247, 0x57DE, 0x8423, 0x41BA, 0x4A88, 0x8F11, 
    0x057C, 0xC0E5, 0xCBD7, 0x0E4E, 0xDDB3, 0x182A, 0x1318, 0xD681, 0xF17B, 0x34E2, 0x3FD0, 0xFA49, 0x29B4, 0xEC2D, 0xE71F, 0x2286, 
    0xA213, 0x678A, 0x6CB8, 0xA921, 0x7ADC, 0xBF45, 0xB477, 0x71EE, 0x5614, 0x938D, 0x98BF, 0x5D26, 0x8EDB, 0x4B42, 0x4070, 0x85E9, 
    0x0F84, 0xCA1D, 0xC12F, 0x04B6, 0xD74B, 0x12D2, 0x19E0, 0xDC79, 0xFB83, 0x3E1A, 0x3528, 0xF0B1, 0x234C, 0xE6D5, 0xEDE7, 0x287E, 
    0xF93D, 0x3CA4, 0x3796, 0xF20F, 0x21F2, 0xE46B, 0xEF59, 0x2AC0, 0x0D3A, 0xC8A3, 0xC391, 0x0608, 0xD5F5, 0x106C, 0x1B5E, 0xDEC7, 
    0x54AA, 0x9133, 0x9A01, 0x5F98, 0x8C65, 0x49FC, 0x42CE, 0x8757, 0xA0AD, 0x6534, 0x6E06, 0xAB9F, 0x7862, 0xBDFB, 0xB6C9, 0x7350, 
    0x51D6, 0x944F, 0x9F7D, 0x5AE4, 0x8919, 0x4C80, 0x47B2, 0x822B, 0xA5D1, 0x6048, 0x6B7A, 0xAEE3, 0x7D1E, 0xB887, 0xB3B5, 0x762C, 
    0xFC41, 0x39D8, 0x32EA, 0xF773, 0x248E, 0xE117, 0xEA25, 0x2FBC, 0x0846, 0xCDDF, 0xC6ED, 0x0374, 0xD089, 0x1510, 0x1E22, 0xDBBB, 
    0x0AF8, 0xCF61, 0xC453, 0x01CA, 0xD237, 0x17AE, 0x1C9C, 0xD905, 0xFEFF, 0x3B66, 0x3054, 0xF5CD, 0x2630, 0xE3A9, 0xE89B, 0x2D02, 
    0xA76F, 0x62F6, 0x69C4, 0xAC5D, 0x7FA0, 0xBA39, 0xB10B, 0x7492, 0x5368, 0x96F1, 0x9DC3, 0x585A, 0x8BA7, 0x4E3E, 0x450C, 0x8095
};

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes);
static TRANSACTION_STATUS_E writeBmbs(uint16_t command, uint8_t *txData, uint32_t numBmbs, PORT_E port);
static TRANSACTION_STATUS_E readBmbs(uint16_t command, uint8_t *rxData, uint32_t numBmbs, PORT_E port);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes)
{
    uint16_t crc = CRC_INITIAL;
    for(int32_t i = 0; i < numBytes; i++)
    {
        // Determine the next look up table index from the current crc and next data byte
        uint8_t index = (uint8_t)((crc >> 7) ^ packet[i]);
        
        // Calculate the next intermediate crc from the current crc and look up table
        crc = (uint16_t)((crc << 8) ^ (uint16_t)(crcTable[index]));
    }
    crc <<= 1;  // Add trailing 0 to 15bit CRC
    return crc;
}

static TRANSACTION_STATUS_E writeBmbs(uint16_t command, uint8_t *txData, uint32_t numBmbs, PORT_E port)
{
    for(int32_t i = 0; i < TRANSACTION_ATTEMPTS; i++)
    {
        // Create a transmit message buffer
        // Size in bytes: Command Word(2) + Command CRC(2) + [Register data(6) + Data CRC(2)] * numBmbs
        uint8_t txBuffer[COMMAND_PACKET_LENGTH + (numBmbs * REGISTER_PACKET_LENGTH)];
        
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
            memcpy(txBuffer + COMMAND_PACKET_LENGTH + (i * REGISTER_PACKET_LENGTH), txData, REGISTER_SIZE_BYTES);
            txBuffer[COMMAND_PACKET_LENGTH + (i * REGISTER_PACKET_LENGTH) + REGISTER_SIZE_BYTES] = (uint8_t)(dataCRC >> BITS_IN_BYTE);;
            txBuffer[COMMAND_PACKET_LENGTH + (i * REGISTER_PACKET_LENGTH) + REGISTER_SIZE_BYTES + 1] = (uint8_t)(dataCRC);;
        }

        // --SPI--
    }
    return TRANSACTION_FAIL;
}

static TRANSACTION_STATUS_E readBmbs(uint16_t command, uint8_t *rxData, uint32_t numBmbs, PORT_E port)
{
    for(int32_t i = 0; i < TRANSACTION_ATTEMPTS; i++)
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
        // goto retry;

        for(int32_t j = 0; j < numBmbs; j++)
        {
            // Extract the register data for each bmb into a temporary array
            uint8_t registerData[REGISTER_SIZE_BYTES];
            memcpy(registerData, rxBuffer + (COMMAND_PACKET_LENGTH + (j * REGISTER_PACKET_LENGTH)), REGISTER_SIZE_BYTES);

            // Extract the CRC sent with the corresponding register data
            uint16_t crc0 = rxBuffer[COMMAND_PACKET_LENGTH + (j * REGISTER_PACKET_LENGTH) + REGISTER_SIZE_BYTES];
            uint16_t crc1 = rxBuffer[COMMAND_PACKET_LENGTH + (j * REGISTER_PACKET_LENGTH) + REGISTER_SIZE_BYTES + 1];
            uint16_t registerCRC = (crc0 << BITS_IN_BYTE) | (crc1);

            // If the CRC is correct for the data sent, populate the rx data buffer with the register data
            // Else, the data is not populated and defaults to zeros
            if(calcCrc(registerData, REGISTER_SIZE_BYTES) == registerCRC)
            {
                memcpy(rxData + (j * REGISTER_SIZE_BYTES), registerData, REGISTER_SIZE_BYTES); 
            }
            else
            {
                goto retry;
            }
        }
        return TRANSACTION_SUCCESS;
        retry:;
    }
    return TRANSACTION_FAIL;
}

static void enumerateBmbs(uint32_t numBmbs)
{
    // Create dummy buffer for read command  
    uint8_t rxBuff[numBmbs * REGISTER_SIZE_BYTES];

    // Attempt to read from an increasing number of bmbs on port A
    // Set chainStatus.portABmbs to the number of bmbs reachable 
    chainStatus.portABmbs = numBmbs;
    for(int32_t i = 1; i <= numBmbs; i++)
    {
        if(!readBmbs(0x0002, rxBuff, i, PORTA))
        {
            chainStatus.portABmbs = (i - 1);
            break;
        }
    }

    // Attempt to read from an increasing number of bmbs on port B
    // Set chainStatus.portBBmbs to the number of bmbs reachable 
    chainStatus.portBBmbs = numBmbs;
    for(int32_t i = 1; i <= numBmbs; i++)
    {
        if(!readBmbs(0x0002, rxBuff, i, PORTB))
        {
            chainStatus.portBBmbs = (i - 1);
            break;
        }
    }

    // Determine the COMMS status from the result of portA and portB enumeration
    if((chainStatus.portABmbs == numBmbs) && (chainStatus.portBBmbs == numBmbs))
    {
        // If there are no chain breaks detected, BIDIRECTIONAL_COMMS are enabled
        chainStatus.commsStatus = BIDIRECTIONAL_COMMS;
    }
    else if((chainStatus.portABmbs + chainStatus.portBBmbs) == numBmbs)
    {
        // If only a single chain break is detected, UNIDIRECTIONAL_COMMS are enabled
        chainStatus.numUniTransactions = 0;
        chainStatus.commsStatus = UNIDIRECTIONAL_COMMS;
    }
    else
    {
        // If multiple chain breaks are detected, LOST_COMMS is set
        chainStatus.commsStatus = LOST_COMMS;
    }
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

TRANSACTION_STATUS_E writeAll(uint16_t command, uint8_t *txData, uint32_t numBmbs)
{ 
    // If comms are lost, immediately attempt to re-establish comms 
    if(chainStatus.commsStatus == LOST_COMMS)
    {
        enumerateBmbs(numBmbs);
    }

    // If bidirectional comms are enabled, attempt bidirectional communication
    if(chainStatus.commsStatus == BIDIRECTIONAL_COMMS)
    {
        // In bidirectional communication, the port swaps every transaction
        // The dataDirection used here for write operations is unique from that used for read operations 
        static PORT_E dataDirection = PORTA;

        // Attempt to write to the bmb chain through the current port set by dataDirection
        if(writeBmbs(command, txData, numBmbs, dataDirection) == TRANSACTION_FAIL)
        {
            // On a transaction failure, attempt to re-establish comms
            enumerateBmbs(numBmbs);
        }
        else
        {
            // On a transaction success, flip the data direction for the next write, and return success
            dataDirection = !dataDirection;
            return TRANSACTION_SUCCESS;
        }
    }

    // If unidirectional comms are enabled, attempt unidirectional communication
    if(chainStatus.commsStatus == UNIDIRECTIONAL_COMMS)
    {
        // Attempt to write to the bmb chain by using both ports to reach every bmb
        TRANSACTION_STATUS_E portAStatus = (chainStatus.portABmbs > 0) ? (writeBmbs(command, txData, chainStatus.portABmbs, PORTA)) : (TRANSACTION_SUCCESS);
        TRANSACTION_STATUS_E portBStatus = (chainStatus.portBBmbs > 0) ? (writeBmbs(command, txData, chainStatus.portBBmbs, PORTB)) : (TRANSACTION_SUCCESS); 
        if((portAStatus == TRANSACTION_FAIL) || (portBStatus == TRANSACTION_FAIL))
        {
            // On a transaction failure, attempt to re-establish comms
            enumerateBmbs(numBmbs);
        }
        else
        {
            // On a transaction success, increment numUniTransactions, and return success
            if(++chainStatus.numUniTransactions >= TRANSACTIONS_BEFORE_RETRY)
            {
                // After TRANSACTIONS_BEFORE_RETRY unidirectional transactions have occured, attempt to re-establish full comms
                // This counter is shared across read and write operations
                enumerateBmbs(numBmbs);
            }
            return TRANSACTION_SUCCESS;
        }
    }

    // Return fail if no communication successful
    return TRANSACTION_FAIL;
}

TRANSACTION_STATUS_E readAll(uint16_t command, uint8_t *rxData, uint32_t numBmbs)
{ 
    // If comms are lost, immediately attempt to re-establish comms 
    if(chainStatus.commsStatus == LOST_COMMS)
    {
        enumerateBmbs(numBmbs);
    }

    // If bidirectional comms are enabled, attempt bidirectional communication
    if(chainStatus.commsStatus == BIDIRECTIONAL_COMMS)
    {
        // In bidirectional communication, the port swaps every transaction
        // The dataDirection used here for read operations is unique from that used for write operations 
        static PORT_E dataDirection = PORTA;

        // Attempt to read from the bmb chain through the current port set by dataDirection
        if(readBmbs(command, rxData, numBmbs, dataDirection) == TRANSACTION_FAIL)
        {
            // On a transaction failure, attempt to re-establish comms
            enumerateBmbs(numBmbs);
        }
        else
        {
            // On a transaction success, flip the data direction for the next read, and return success
            dataDirection = !dataDirection;
            return TRANSACTION_SUCCESS;
        }
    }

    // If unidirectional comms are enabled, attempt unidirectional communication
    if(chainStatus.commsStatus == UNIDIRECTIONAL_COMMS)
    {
        // Attempt to write to the bmb chain by using both ports to reach every bmb
        TRANSACTION_STATUS_E portAStatus = (chainStatus.portABmbs > 0) ? (readBmbs(command, rxData, chainStatus.portABmbs, PORTA)) : (TRANSACTION_SUCCESS);
        TRANSACTION_STATUS_E portBStatus = (chainStatus.portBBmbs > 0) ? (readBmbs(command, rxData, chainStatus.portBBmbs, PORTB)) : (TRANSACTION_SUCCESS); 
        if((portAStatus == TRANSACTION_FAIL) || (portBStatus == TRANSACTION_FAIL))
        {
            // On a transaction failure, attempt to re-establish comms
            enumerateBmbs(numBmbs);
        }
        else
        {
            // On a transaction success, increment numUniTransactions, and return success
            if(++chainStatus.numUniTransactions >= TRANSACTIONS_BEFORE_RETRY)
            {
                // After TRANSACTIONS_BEFORE_RETRY unidirectional transactions have occured, attempt to re-establish full comms
                // This counter is shared across read and write operations
                enumerateBmbs(numBmbs);
            }
            return TRANSACTION_SUCCESS;
        }
    }

    // Return fail if no communication successful
    return TRANSACTION_FAIL;
}
