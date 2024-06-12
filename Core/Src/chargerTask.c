#include "chargerTask.h"
#include "charger.h"
#include "main.h"
#include "debug.h"
#include "cmsis_os.h"
#include "GopherCAN.h"
#include "gopher_sense.h"

extern volatile bool chargerConnected;
extern volatile bool newChargerMessage;
extern volatile bool EVSEConnected;
extern volatile uint32_t EVSELastUpdate;

extern CAN_HandleTypeDef hcan1;

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

void initChargerTask()
{

}

void runChargerTask()
{
    static uint32_t currentLimit = DEFAULT_CURRENT_LIMIT_A;

    //if J1772
    if(EVSEConnected)
    {
        HAL_GPIO_WritePin(CP_EN_GPIO_Port, CP_EN_Pin, GPIO_PIN_SET);
        if((HAL_GetTick() - EVSELastUpdate) > 10)
        {
            EVSEConnected = false;
            HAL_GPIO_WritePin(CP_EN_GPIO_Port, CP_EN_Pin, GPIO_PIN_RESET);
        }
    }

    if(chargerStatusByte.data != 0x00)
    {
        chargerConnected = true;
    }

    if(chargerConnected)
    {
        // Take in data
        Charger_Data_S chargerDataLocal;
        taskENTER_CRITICAL();
        chargerDataLocal = chargerTaskOutputData;
        float maxCellVoltage = bmbTaskOutputData.maxCellVoltage;
        float minCellVoltage = bmbTaskOutputData.minCellVoltage;
        float packVoltage = bmbTaskOutputData.accumulatorVoltage;
        taskEXIT_CRITICAL();

        // Voltage and current request defauult to 0
		float voltageRequest = 0.0f;
		float currentRequest = 0.0f;
		bool chargeOkay = false;

        // Cell Imbalance hysteresis
        // cellImbalancePresent set when pack cell imbalance exceeds high threshold
        // cellImbalancePresent reset when pack cell imbalance falls under low threshold
        static bool cellImbalancePresent = false;
        float cellImbalance = maxCellVoltage - minCellVoltage;
        if(cellImbalance > MAX_CELL_IMBALANCE_THRES_HIGH)
        {
            cellImbalancePresent = true;
        }
        else if(cellImbalance < MAX_CELL_IMBALANCE_THRES_LOW)
        {
            cellImbalancePresent = false;
        }
        else
        {
            // Maintain state of cellImbalancePresent
        }

        // Cell Over Voltage hysteresis
        // cellOverVoltagePresent set when pack max cell voltage exceeds high threshold
        // cellOverVoltagePresent reset when pack max cell voltage falls under low threshold
        static bool cellOverVoltagePresent = false;
        if(maxCellVoltage > MAX_CELL_VOLTAGE_THRES_HIGH)
        {
            cellOverVoltagePresent = true;
        }
        else if(maxCellVoltage < MAX_CELL_VOLTAGE_THRES_LOW)
        {
            cellOverVoltagePresent = false;
        }
        else
        {
            // Maintain state of cellOverVoltagePresent
        }

        // Charging is only allowed if the bms disable, cellImbalancePresent, and cellOverVoltagePresent are all not true 
        chargeOkay = !(cellImbalancePresent || cellOverVoltagePresent);
        if(chargeOkay)
        {
            // Always request max charging voltage
            voltageRequest = MAX_CHARGE_VOLTAGE_V;

            currentRequest = currentLimit;

            // Set current request
            // Calculate the max current possible given the charger power and pack voltage
            const float powerLimitAmps = (CHARGER_OUTPUT_POWER_W) / (packVoltage + CHARGER_VOLTAGE_MISMATCH_THRESHOLD);

            // Limit the charge current to the charger max power output if necessary 
            if(currentRequest > powerLimitAmps)
            {
                currentRequest = powerLimitAmps;
            }

            if(maxCellVoltage > CELL_VOLTAGE_TAPER_THRESHOLD)
            {
                currentRequest = currentLimit * ((MAX_BRICK_VOLTAGE - maxCellVoltage) / (MAX_BRICK_VOLTAGE - CELL_VOLTAGE_TAPER_THRESHOLD));

            }            
        }

        // Send the calculated voltage and current requests to the charger over CAN
        sendChargerMessage(voltageRequest, currentRequest, chargeOkay);

        // Pass out data
        taskENTER_CRITICAL();
        chargerTaskOutputData = chargerDataLocal;
        taskEXIT_CRITICAL();
    }    

}


