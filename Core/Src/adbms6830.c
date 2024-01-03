/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "stm32f4xx_hal.h"
#include "adbms6830.h"
#include "spi.h"

/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define CRC_SIZE_BYTES      2
#define CRC_LUT_SIZE        256

#define CRC_CMD_SEED        0x0010
#define CRC_CMD_POLY        0x4599
#define CRC_CMD_SIZE        15

#define CRC_DATA_SEED       0x0010
#define CRC_DATA_POLY       0x008F
#define CRC_DATA_SIZE       10

#define COMMAND_PACKET_LENGTH    (COMMAND_SIZE_BYTES + CRC_SIZE_BYTES)
#define REGISTER_PACKET_LENGTH   (REGISTER_SIZE_BYTES + CRC_SIZE_BYTES)

#define TRANSACTION_ATTEMPTS    3
#define SPI_TIMEOUT_MS          10

/* ==================================================================== */
/* ======================= EXTERNAL VARIABLES ========================= */
/* ==================================================================== */

extern SPI_HandleTypeDef hspi1;

/* ==================================================================== */
/* ========================= ENUMERATED TYPES========================== */
/* ==================================================================== */

typedef enum
{
    PORTA = 0,
    PORTB,
    NUM_PORTS
} PORT_E;

typedef enum
{
    MULTIPLE_CHAIN_BREAK = 0,
    SINGLE_CHAIN_BREAK,
    CHAIN_COMPLETE
} CHAIN_STATUS_E; 

typedef enum
{
    COMMAND_CRC = 0,
    DATA_CRC,
    NUM_CRC_TYPES
} CRC_TYPE_E; 

/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */

typedef struct
{
    CHAIN_STATUS_E chainStatus;
    uint8_t availableBmbs[NUM_PORTS];
} CHAIN_INFO_S;

typedef struct
{
    uint16_t seed;
    uint16_t poly;
    uint16_t size;
    uint16_t bitShift;
    uint16_t table[CRC_LUT_SIZE];
} CRC_DATA_S;

CRC_DATA_S crcData[NUM_CRC_TYPES] =
{
    {
        .seed = CRC_CMD_SEED,
        .poly = CRC_CMD_POLY,
        .size = CRC_CMD_SIZE,
        .bitShift = CRC_CMD_SIZE - BITS_IN_BYTE
    },
    {
        .seed = CRC_DATA_SEED,
        .poly = CRC_DATA_POLY,
        .size = CRC_DATA_SIZE,
        .bitShift = CRC_DATA_SIZE - BITS_IN_BYTE
    }
};

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

static CHAIN_INFO_S chainInfo;

static uint8_t commandCounter = 0;

/* ==================================================================== */
/* =================== LOCAL FUNCTION DECLARATIONS ==================== */
/* ==================================================================== */

static void openPort(PORT_E port);
static void closePort(PORT_E port);
static void resetCommandCounter();
static void incCommandCounter();
static void initCrcTables();
static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes, CRC_TYPE_E crcType);
static uint16_t calcCmdCrc(uint8_t *packet, uint32_t numBytes);
static uint16_t calcDataCrc(uint8_t *packet, uint32_t numBytes, bool useCommandCounter);
static TRANSACTION_STATUS_E sendCommand(uint16_t command, uint32_t numBmbs, PORT_E port);
static TRANSACTION_STATUS_E writeRegister(uint16_t command, uint8_t *txBuffer, uint32_t numBmbs, PORT_E port);
static TRANSACTION_STATUS_E readRegister(uint16_t command, uint8_t *rxBuffer, uint32_t numBmbs, PORT_E port);
static void enumerateBmbs(uint32_t numBmbs);

/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

static void openPort(PORT_E port)
{
    if(port == PORTA)
    {        
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);

        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);

        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);

        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
    }
    else if(port == PORTB)
    {
        HAL_GPIO_WritePin(PORTB_CS_GPIO_Port, PORTB_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTB_CS_GPIO_Port, PORTB_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);

        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);

        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);
        
        HAL_GPIO_WritePin(PORTB_CS_GPIO_Port, PORTB_CS_Pin, GPIO_PIN_RESET);
    }
}

static void closePort(PORT_E port)
{
    if(port == PORTA)
    {        
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
    }
    else if(port == PORTB)
    {
        HAL_GPIO_WritePin(PORTB_CS_GPIO_Port, PORTB_CS_Pin, GPIO_PIN_SET);
    }
}

static void resetCommandCounter()
{
    commandCounter = 0;
}

static void incCommandCounter()
{
    commandCounter++;
    if(commandCounter > 63)
    {
        commandCounter = 1;
    }
}

static void initCrcTables()
{
    for(int32_t crcType = 0; crcType < NUM_CRC_TYPES; crcType++)
    {
        uint16_t shiftBits = crcData[crcType].bitShift;
        uint16_t bitMask = (uint16_t)1 << (crcData[crcType].size - 1);

        for (uint16_t dividend = 0; dividend < 256; dividend++) /* iterate over all possible input byte values 0 - 255 */
        {
            uint16_t curByte = (dividend << shiftBits); /* move dividend byte into MSB of 16Bit CRC */

            for (int32_t bit = 0; bit < 8; bit++)
            {
                if (curByte & bitMask)
                {
                    curByte <<= 1;
                    curByte ^= crcData[crcType].poly;
                }
                else
                {
                    curByte <<= 1;
                }
            }
            crcData[crcType].table[dividend] = curByte;
        }
    }
}

static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes, CRC_TYPE_E crcType)
{
    uint16_t crc = crcData[crcType].seed;
    for(int32_t i = 0; i < numBytes; i++)
    {
        // Determine the next look up table index from the current crc and next data byte
        uint8_t index = (uint8_t)((crc >> (crcData[crcType].bitShift)) ^ packet[i]);
        
        // Calculate the next intermediate crc from the current crc and look up table
        crc = (uint16_t)((crc << 8) ^ (uint16_t)(crcData[crcType].table[index]));
    }
    return crc;
}

static uint16_t calcCmdCrc(uint8_t *packet, uint32_t numBytes)
{
    uint16_t crc = calcCrc(packet, numBytes, COMMAND_CRC);
    crc <<= 1;
    return crc;
}

static uint16_t calcDataCrc(uint8_t *packet, uint32_t numBytes, bool useCommandCounter)
{
    uint16_t crc = calcCrc(packet, numBytes, DATA_CRC);

    if(useCommandCounter)
    {
        crc ^= ((uint16_t)commandCounter << 4);
    }

    uint16_t bitMask = (uint16_t)1 << (crcData[DATA_CRC].size - 1);
    for (int32_t bit = 0; bit < 6; bit++)
    {
        if (crc & bitMask)
        {
            crc <<= 1;
            crc ^= crcData[DATA_CRC].poly;
        }
        else
        {
            crc <<= 1;
        }
    }
    crc &= 0x03FF;
    if(useCommandCounter)
    {
        crc |= ((uint16_t)commandCounter << 10);
    }
    return crc;
}


static TRANSACTION_STATUS_E sendCommand(uint16_t command, uint32_t numBmbs, PORT_E port)
{
    // Size in bytes: Command Word(2) + Command CRC(2)
    uint32_t packetLength = COMMAND_PACKET_LENGTH;

    // Create a transmit message buffer and a dummy rx buffer
    uint8_t txBuffer[packetLength];
    uint8_t rxBuffer[packetLength];

    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCmdCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    // SPIify
    openPort(port);
    if(SPI_TRANSMIT(HAL_SPI_TransmitReceive_IT, &hspi1, SPI_TIMEOUT_MS, txBuffer, rxBuffer, packetLength) != SPI_SUCCESS)
    {
        closePort(port);
        return TRANSACTION_FAIL;
    }
    closePort(port);
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E writeRegister(uint16_t command, uint8_t *txBuff, uint32_t numBmbs, PORT_E port)
{
    // Size in bytes: Command Word(2) + Command CRC(2) + [Register data(6) + Data CRC(2)] * numBmbs
    uint32_t packetLength = COMMAND_PACKET_LENGTH + (numBmbs * REGISTER_PACKET_LENGTH);

    // Create a transmit message buffer and a dummy rx buffer
    uint8_t txBuffer[packetLength];
    uint8_t rxBuffer[packetLength];

    // Clear buffer arrays
    memset(txBuffer, 0, packetLength);
    memset(rxBuffer, 0, packetLength);
    
    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCmdCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    // Calculate the CRC on the register data packet (2 byte CRC on 6 byte packet)
    uint16_t dataCRC = calcDataCrc(txBuff, REGISTER_SIZE_BYTES, false);

    // For each bmb, append a copy of the register data and corresponding CRC to the tx buffer  
    for(int32_t i = 0; i < numBmbs; i++)
    {
        memcpy(txBuffer + COMMAND_PACKET_LENGTH + (i * REGISTER_PACKET_LENGTH), txBuff, REGISTER_SIZE_BYTES);
        txBuffer[COMMAND_PACKET_LENGTH + (i * REGISTER_PACKET_LENGTH) + REGISTER_SIZE_BYTES] = (uint8_t)(dataCRC >> BITS_IN_BYTE);;
        txBuffer[COMMAND_PACKET_LENGTH + (i * REGISTER_PACKET_LENGTH) + REGISTER_SIZE_BYTES + 1] = (uint8_t)(dataCRC);;
    }

    // SPIify
    openPort(port);
    if(SPI_TRANSMIT(HAL_SPI_TransmitReceive_IT, &hspi1, SPI_TIMEOUT_MS, txBuffer, rxBuffer, packetLength) != SPI_SUCCESS)
    {
        closePort(port);
        return TRANSACTION_FAIL;
    }
    closePort(port);
    incCommandCounter();
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E readRegister(uint16_t command, uint8_t *rxBuff, uint32_t numBmbs, PORT_E port)
{
    // Size in bytes: Command Word(2) + Command CRC(2) + [Register data(6) + Data CRC(2)] * numBmbs
    uint32_t packetLength = COMMAND_PACKET_LENGTH + (numBmbs * REGISTER_PACKET_LENGTH);

    // Create transmit and recieve message buffers
    uint8_t txBuffer[packetLength];
    uint8_t rxBuffer[packetLength];

    // Clear buffer arrays
    // Do I Care?? Rx probably not
    memset(txBuffer, 0, packetLength);
    memset(rxBuffer, 0, packetLength);
    
    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCmdCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    for(int32_t i = 0; i < TRANSACTION_ATTEMPTS; i++)
    {
        openPort(port);
        if(SPI_TRANSMIT(HAL_SPI_TransmitReceive_IT, &hspi1, SPI_TIMEOUT_MS, txBuffer, rxBuffer, packetLength) != SPI_SUCCESS)
        {
            closePort(port);
            return TRANSACTION_FAIL;
        }
        closePort(port);

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
            if(calcDataCrc(registerData, REGISTER_SIZE_BYTES, true) != registerCRC)
            {
                goto retry;
            }
            if(port == PORTA)
            {
                memcpy(rxBuff + (j * REGISTER_SIZE_BYTES), registerData, REGISTER_SIZE_BYTES); 
            }
            else if(port == PORTB)
            {
                memcpy(rxBuff + ((numBmbs - j - 1) * REGISTER_SIZE_BYTES), registerData, REGISTER_SIZE_BYTES); 
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

    // Attempt to read from an increasing number of bmbs from each port
    // Set availableBmbs to the number of bmbs reachable 
    for(int32_t port = 0; port < NUM_PORTS; port++)
    {
        chainInfo.availableBmbs[port] = numBmbs;
        for(int32_t bmbs = 1; bmbs <= numBmbs; bmbs++)
        {
            if(!readRegister(0x002C, rxBuff, bmbs, port))
            {
                chainInfo.availableBmbs[port] = (bmbs - 1);
                break;
            }
        }
    }

    // Determine the COMMS status from the result of portA and portB enumeration
    if((chainInfo.availableBmbs[PORTA] == numBmbs) && (chainInfo.availableBmbs[PORTB] == numBmbs))
    {
        // If there are no chain breaks detected, BIDIRECTIONAL_COMMS are enabled
        chainInfo.chainStatus = CHAIN_COMPLETE;
    }
    else if((chainInfo.availableBmbs[PORTA] + chainInfo.availableBmbs[PORTB]) == numBmbs)
    {
        // If only a single chain break is detected, UNIDIRECTIONAL_COMMS are enabled
        chainInfo.chainStatus = SINGLE_CHAIN_BREAK;
    }
    else
    {
        // If multiple chain breaks are detected, LOST_COMMS is set
        chainInfo.chainStatus = MULTIPLE_CHAIN_BREAK;
    }
}

// /* ==================================================================== */
// /* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
// /* ==================================================================== */

// TRANSACTION_STATUS_E commandAll(uint16_t command, uint32_t numBmbs)
// { 
//     // Communication will only be repeated if the first attempt fails and
//     // the first consecutive enumeration event re-establishes communication
//     // This allows comms to remain stable if a new chain break occurs between transactions
//     for(int32_t i = 0; i < 2; i++)
//     {
//         if(chainInfo.chainStatus == CHAIN_COMPLETE)
//         {
//             // If the chain is complete, swap the origin port every write transaction
//             static PORT_E originPort = PORTA;
//             if(sendCommand(command, numBmbs, PORTA) == TRANSACTION_SUCCESS)
//             {
//                 // On a transaction success, swap origin port, and return success
//                 originPort = !originPort;
//                 return TRANSACTION_SUCCESS;
//             }
//         }
//         else
//         {
//             // If there are any chain breaks, use both ports to reach as many bmbs as possible
//             TRANSACTION_STATUS_E portAStatus = (chainInfo.availableBmbs[PORTA] > 0) ? (sendCommand(command, chainInfo.availableBmbs[PORTA], PORTA)) : (TRANSACTION_SUCCESS);
//             TRANSACTION_STATUS_E portBStatus = (chainInfo.availableBmbs[PORTB] > 0) ? (sendCommand(command, chainInfo.availableBmbs[PORTB], PORTB)) : (TRANSACTION_SUCCESS); 

//             if((portAStatus == TRANSACTION_SUCCESS) && (portBStatus == TRANSACTION_SUCCESS))
//             {
//                 if(chainInfo.chainStatus == SINGLE_CHAIN_BREAK)
//                 {
//                     // If every bmb is successfully reached, re-enumerate and return success
//                     // enumerateBmbs(numBmbs);
//                     return TRANSACTION_SUCCESS; 
//                 }
//                 else
//                 {
//                     // If not every bmb is successfully reached, re-enumerate and return failure
//                     enumerateBmbs(numBmbs);
//                     return TRANSACTION_FAIL;
//                 }
//             }
//         }
//         // If the bms fails to communicate with the expected number of bmbs,
//         // re-enumerate and retry a maximum of 1 time
//         enumerateBmbs(numBmbs);
//     }
//     return TRANSACTION_FAIL;
// }

TRANSACTION_STATUS_E writeAll(uint16_t command, uint8_t *txData, uint32_t numBmbs)
{ 
    // writeRegister(command, txData, numBmbs, PORTA);


    // Communication will only be repeated if the first attempt fails and
    // the first consecutive enumeration event re-establishes communication
    // This allows comms to remain stable if a new chain break occurs between transactions
    for(int32_t i = 0; i < 2; i++)
    {
        if(chainInfo.chainStatus == CHAIN_COMPLETE)
        {
            // If the chain is complete, swap the origin port every write transaction
            static PORT_E originPort = PORTA;
            if(writeRegister(command, txData, numBmbs, originPort) == TRANSACTION_SUCCESS)
            {
                // On a transaction success, swap origin port, and return success
                originPort = !originPort;
                return TRANSACTION_SUCCESS;
            }
        }
        else
        {
            // If there are any chain breaks, use both ports to reach as many bmbs as possible
            TRANSACTION_STATUS_E portAStatus = (chainInfo.availableBmbs[PORTA] > 0) ? (writeRegister(command, txData, chainInfo.availableBmbs[PORTA], PORTA)) : (TRANSACTION_SUCCESS);
            TRANSACTION_STATUS_E portBStatus = (chainInfo.availableBmbs[PORTB] > 0) ? (writeRegister(command, txData, chainInfo.availableBmbs[PORTB], PORTB)) : (TRANSACTION_SUCCESS); 

            if((portAStatus == TRANSACTION_SUCCESS) && (portBStatus == TRANSACTION_SUCCESS))
            {
                enumerateBmbs(numBmbs);
                if(chainInfo.chainStatus == SINGLE_CHAIN_BREAK)
                {
                    // If every bmb is successfully reached, re-enumerate and return success
                    return TRANSACTION_SUCCESS; 
                }
                else
                {
                    // If not every bmb is successfully reached, re-enumerate and return failure
                    return TRANSACTION_FAIL;
                }
            }
        }
        // If the bms fails to communicate with the expected number of bmbs,
        // re-enumerate and retry a maximum of 1 time
        enumerateBmbs(numBmbs);
    }
    return TRANSACTION_FAIL;
}

TRANSACTION_STATUS_E readAll(uint16_t command, uint8_t *rxData, uint32_t numBmbs)
{ 
    static bool init = true;
    if(init)
    {
        // closePort(PORTB);
        initCrcTables();
    }
    // readRegister(command, rxData, numBmbs, PORTA);

    // Communication will only be repeated if the first attempt fails and
    // the first consecutive enumeration event re-establishes communication
    // This allows comms to remain stable if a new chain break occurs between transactions
    for(int32_t i = 0; i < 2; i++)
    {
        if(chainInfo.chainStatus == CHAIN_COMPLETE)
        {
            // If the chain is complete, swap the origin port every read transaction
            static PORT_E originPort = PORTA;
            if(readRegister(command, rxData, numBmbs, originPort) == TRANSACTION_SUCCESS)
            {
                // On a transaction success, swap origin port, and return success
                originPort = !originPort;
                return TRANSACTION_SUCCESS;
            }
        }
        else
        {
            // If there are any chain breaks, use both ports to reach as many bmbs as possible
            TRANSACTION_STATUS_E portAStatus = (chainInfo.availableBmbs[PORTA] > 0) ? (readRegister(command, rxData, chainInfo.availableBmbs[PORTA], PORTA)) : (TRANSACTION_SUCCESS);
            TRANSACTION_STATUS_E portBStatus = (chainInfo.availableBmbs[PORTB] > 0) ? (readRegister(command, rxData + REGISTER_SIZE_BYTES * (numBmbs - chainInfo.availableBmbs[PORTB]), chainInfo.availableBmbs[PORTB], PORTB)) : (TRANSACTION_SUCCESS); 

            if((portAStatus == TRANSACTION_SUCCESS) && (portBStatus == TRANSACTION_SUCCESS))
            {
                enumerateBmbs(numBmbs);
                if(chainInfo.chainStatus == SINGLE_CHAIN_BREAK)
                {
                    // If every bmb is successfully reached, re-enumerate and return success
                    return TRANSACTION_SUCCESS; 
                }
                else
                {
                    // If not every bmb is successfully reached, re-enumerate and return failure
                    return TRANSACTION_FAIL;
                }
            }
        }
        // If communication with the expected number of bmbs fails,
        // re-enumerate and retry a maximum of 1 time
        enumerateBmbs(numBmbs);
    }
    return TRANSACTION_FAIL;
}
