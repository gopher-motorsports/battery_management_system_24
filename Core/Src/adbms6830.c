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

#define CRC_INITIAL         0x0020
#define CRC_POLYNOMIAL      0x4599
#define CRC_LUT_SIZE        256
#define CRC_SIZE_BYTES           2

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

/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */

typedef struct
{
    CHAIN_STATUS_E chainStatus;
    uint8_t availableBmbs[NUM_PORTS];
} CHAIN_INFO_S;

/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

CHAIN_INFO_S chainInfo;

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

static void openPort(PORT_E port);
static void closePort(PORT_E port);
static uint16_t calcCrc(uint8_t *packet, uint32_t numBytes);
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
    }
    else if(port == PORTB)
    {
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
        vTaskDelay(1 * portTICK_PERIOD_MS);
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_RESET);
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
        HAL_GPIO_WritePin(PORTA_CS_GPIO_Port, PORTA_CS_Pin, GPIO_PIN_SET);
    }
}

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
    uint16_t commandCRC = calcCrc(txBuffer, COMMAND_SIZE_BYTES);
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
    
    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    // Calculate the CRC on the register data packet (2 byte CRC on 6 byte packet)
    uint16_t dataCRC = calcCrc(txBuffer, REGISTER_SIZE_BYTES);

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
    return TRANSACTION_SUCCESS;
}

static TRANSACTION_STATUS_E readRegister(uint16_t command, uint8_t *rxBuff, uint32_t numBmbs, PORT_E port)
{
    // Size in bytes: Command Word(2) + Command CRC(2) + [Register data(6) + Data CRC(2)] * numBmbs
    uint32_t packetLength = COMMAND_PACKET_LENGTH + (numBmbs * REGISTER_PACKET_LENGTH);

    // Create transmit and recieve message buffers
    uint8_t txBuffer[packetLength];
    uint8_t rxBuffer[packetLength];
    
    // Populate the tx buffer with the command word
    txBuffer[0] = (uint8_t)(command >> BITS_IN_BYTE);
    txBuffer[1] = (uint8_t)(command);

    // Calculate the CRC on the command word and place the tx buffer
    uint16_t commandCRC = calcCrc(txBuffer, COMMAND_SIZE_BYTES);
    txBuffer[2] = (uint8_t)(commandCRC >> BITS_IN_BYTE);
    txBuffer[3] = (uint8_t)(commandCRC);

    for(int32_t i = 0; i < TRANSACTION_ATTEMPTS; i++)
    {
        openPort(port);
        if(SPI_TRANSMIT(HAL_SPI_TransmitReceive_IT, &hspi1, SPI_TIMEOUT_MS, txBuffer, rxBuff, packetLength) != SPI_SUCCESS)
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
            if(calcCrc(registerData, REGISTER_SIZE_BYTES) != registerCRC)
            {
                goto retry;
            }
            if(port == PORTA)
            {
                memcpy(rxBuffer + (j * REGISTER_SIZE_BYTES), registerData, REGISTER_SIZE_BYTES); 
            }
            else if(port == PORTB)
            {
                memcpy(rxBuffer + ((numBmbs - j - 1) * REGISTER_SIZE_BYTES), registerData, REGISTER_SIZE_BYTES); 
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
            if(!readRegister(0x0002, rxBuff, bmbs, PORTA))
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

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

TRANSACTION_STATUS_E commandAll(uint16_t command, uint32_t numBmbs)
{ 
    // Communication will only be repeated if the first attempt fails and
    // the first consecutive enumeration event re-establishes communication
    // This allows comms to remain stable if a new chain break occurs between transactions
    for(int32_t i = 0; i < 2; i++)
    {
        if(chainInfo.chainStatus == CHAIN_COMPLETE)
        {
            // If the chain is complete, swap the origin port every write transaction
            static PORT_E originPort = PORTA;
            if(sendCommand(command, numBmbs, PORTA) == TRANSACTION_SUCCESS)
            {
                // On a transaction success, swap origin port, and return success
                originPort = !originPort;
                return TRANSACTION_SUCCESS;
            }
        }
        else
        {
            // If there are any chain breaks, use both ports to reach as many bmbs as possible
            TRANSACTION_STATUS_E portAStatus = (chainInfo.availableBmbs[PORTA] > 0) ? (sendCommand(command, chainInfo.availableBmbs[PORTA], PORTA)) : (TRANSACTION_SUCCESS);
            TRANSACTION_STATUS_E portBStatus = (chainInfo.availableBmbs[PORTB] > 0) ? (sendCommand(command, chainInfo.availableBmbs[PORTB], PORTB)) : (TRANSACTION_SUCCESS); 

            if((portAStatus == TRANSACTION_SUCCESS) && (portBStatus == TRANSACTION_SUCCESS))
            {
                if(chainInfo.chainStatus == SINGLE_CHAIN_BREAK)
                {
                    // If every bmb is successfully reached, re-enumerate and return success
                    enumerateBmbs(numBmbs);
                    return TRANSACTION_SUCCESS; 
                }
                else
                {
                    // If not every bmb is successfully reached, re-enumerate and return failure
                    enumerateBmbs(numBmbs);
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

TRANSACTION_STATUS_E writeAll(uint16_t command, uint8_t *txData, uint32_t numBmbs)
{ 
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
                if(chainInfo.chainStatus == SINGLE_CHAIN_BREAK)
                {
                    // If every bmb is successfully reached, re-enumerate and return success
                    enumerateBmbs(numBmbs);
                    return TRANSACTION_SUCCESS; 
                }
                else
                {
                    // If not every bmb is successfully reached, re-enumerate and return failure
                    enumerateBmbs(numBmbs);
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
                if(chainInfo.chainStatus == SINGLE_CHAIN_BREAK)
                {
                    // If every bmb is successfully reached, re-enumerate and return success
                    enumerateBmbs(numBmbs);
                    return TRANSACTION_SUCCESS; 
                }
                else
                {
                    // If not every bmb is successfully reached, re-enumerate and return failure
                    enumerateBmbs(numBmbs);
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
