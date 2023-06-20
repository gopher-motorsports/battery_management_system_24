/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

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
    TRANSACTION_FAIL = 0 ,
    TRANSACTION_SUCCESS
} TRANSACTION_STATUS_E;

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
        if( (writeBmbs(command, txData, chainStatus.portABmbs, PORTA) == TRANSACTION_FAIL) ||
            (writeBmbs(command, txData, chainStatus.portBBmbs, PORTB) == TRANSACTION_FAIL))
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
        // Attempt to read from the bmb chain by using both ports to reach every bmb 
        if( (writeBmbs(command, rxData, chainStatus.portABmbs, PORTA) == TRANSACTION_FAIL) ||
            (writeBmbs(command, rxData, chainStatus.portBBmbs, PORTB) == TRANSACTION_FAIL))
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
