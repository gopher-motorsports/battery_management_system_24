#include "currentSenseTask.h"
#include "currentSense.h"
#include "main.h"
#include "cmsis_os.h"

void initCurrentSenseTask()
{

}

void runCurrentSenseTask()
{
    CurrentSenseTaskOutputData_S currentSenseOutputDataLocal;

    taskENTER_CRITICAL();
    float minCellVoltage = bmbTaskOutputData.minCellVoltage;
    taskEXIT_CRITICAL();

    getTractiveSystemCurrent(&currentSenseOutputDataLocal);

    static uint32_t lastSocAndSoeUpdate = 0;
    uint32_t deltaTimeMs = HAL_GetTick() - lastSocAndSoeUpdate;
    lastSocAndSoeUpdate = HAL_GetTick();

    // Populate data to be used in SOC and SOE calculation
    Soc_S* pSoc = &currentSenseOutputDataLocal.soc;
    pSoc->minBrickVoltage = minCellVoltage;
    pSoc->curAccumulatorCurrent = currentSenseOutputDataLocal.tractiveSystemCurrent;
    pSoc->deltaTimeMs = deltaTimeMs;
    updateSocAndSoe(&currentSenseOutputDataLocal.soc);

    taskENTER_CRITICAL();
    currentSenseOutputData = currentSenseOutputDataLocal;
    taskEXIT_CRITICAL();

}
