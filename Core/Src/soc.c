/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "cellData.h"
#include "packData.h"
#include "soc.h"
#include "lookupTable.h"
#include "math.h"


/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

#define TABLE_LENGTH 101


/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

// State of charge for VTC6 cells
float stateOfCharge[TABLE_LENGTH] = 
{
    0.0000f, 0.0100f, 0.0200f, 0.0300f, 0.0400f, 0.0500f, 0.0600f, 0.0700f, 0.0800f, 0.0900f, 0.1000f, 
    0.1100f, 0.1200f, 0.1300f, 0.1400f, 0.1500f, 0.1600f, 0.1700f, 0.1800f, 0.1900f, 0.2000f, 0.2100f, 
    0.2200f, 0.2300f, 0.2400f, 0.2500f, 0.2600f, 0.2700f, 0.2800f, 0.2900f, 0.3000f, 0.3100f, 0.3200f, 
    0.3300f, 0.3400f, 0.3500f, 0.3600f, 0.3700f, 0.3800f, 0.3900f, 0.4000f, 0.4100f, 0.4200f, 0.4300f, 
    0.4400f, 0.4500f, 0.4600f, 0.4700f, 0.4800f, 0.4900f, 0.5000f, 0.5100f, 0.5200f, 0.5300f, 0.5400f, 
    0.5500f, 0.5600f, 0.5700f, 0.5800f, 0.5900f, 0.6000f, 0.6100f, 0.6200f, 0.6300f, 0.6400f, 0.6500f, 
    0.6600f, 0.6700f, 0.6800f, 0.6900f, 0.7000f, 0.7100f, 0.7200f, 0.7300f, 0.7400f, 0.7500f, 0.7600f, 
    0.7700f, 0.7800f, 0.7900f, 0.8000f, 0.8100f, 0.8200f, 0.8300f, 0.8400f, 0.8500f, 0.8600f, 0.8700f, 
    0.8800f, 0.8900f, 0.9000f, 0.9100f, 0.9200f, 0.9300f, 0.9400f, 0.9500f, 0.9600f, 0.9700f, 0.9800f, 
    0.9900f, 1.0000f
};

// Open cell voltage for VTC6 cells. References SOC table
float openCellVoltage[TABLE_LENGTH] = 
{
    2.4833f, 2.6490f, 2.7794f, 2.8712f, 2.9375f, 2.9893f, 3.0316f, 3.0676f, 3.0982f, 3.1251f, 3.1509f, 
    3.1757f, 3.2010f, 3.2272f, 3.2541f, 3.2803f, 3.3053f, 3.3290f, 3.3514f, 3.3718f, 3.3904f, 3.4082f, 
    3.4253f, 3.4405f, 3.4596f, 3.4804f, 3.4917f, 3.4997f, 3.5067f, 3.5135f, 3.5202f, 3.5271f, 3.5349f, 
    3.5482f, 3.5643f, 3.5787f, 3.5940f, 3.6077f, 3.6197f, 3.6318f, 3.6430f, 3.6542f, 3.6642f, 3.6738f, 
    3.6832f, 3.6931f, 3.7025f, 3.7117f, 3.7213f, 3.7310f, 3.7403f, 3.7491f, 3.7582f, 3.7673f, 3.7759f, 
    3.7841f, 3.7918f, 3.8003f, 3.8079f, 3.8152f, 3.8219f, 3.8290f, 3.8355f, 3.8423f, 3.8493f, 3.8562f, 
    3.8637f, 3.8724f, 3.8822f, 3.8941f, 3.9072f, 3.9206f, 3.9335f, 3.9463f, 3.9598f, 3.9728f, 3.9857f, 
    3.9979f, 4.0099f, 4.0219f, 4.0320f, 4.0413f, 4.0504f, 4.0566f, 4.0622f, 4.0660f, 4.0691f, 4.0723f, 
    4.0756f, 4.0784f, 4.0819f, 4.0855f, 4.0898f, 4.0948f, 4.1006f, 4.1087f, 4.1177f, 4.1298f, 4.1445f, 
    4.1642f, 4.2000f
};

// State of energy for VTC6 cells. References SOC table
float stateOfEnergy[TABLE_LENGTH] = 
{
    0.0000f, 0.0070f, 0.0144f, 0.0221f, 0.0300f, 0.0381f, 0.0462f, 0.0545f, 0.0629f, 0.0714f, 0.0799f, 
    0.0886f, 0.0972f, 0.1060f, 0.1148f, 0.1237f, 0.1326f, 0.1417f, 0.1508f, 0.1599f, 0.1691f, 0.1784f, 
    0.1877f, 0.1970f, 0.2064f, 0.2158f, 0.2253f, 0.2348f, 0.2444f, 0.2539f, 0.2635f, 0.2731f, 0.2827f, 
    0.2923f, 0.3020f, 0.3117f, 0.3215f, 0.3313f, 0.3411f, 0.3510f, 0.3609f, 0.3708f, 0.3808f, 0.3907f, 
    0.4008f, 0.4108f, 0.4209f, 0.4309f, 0.4411f, 0.4512f, 0.4614f, 0.4716f, 0.4818f, 0.4920f, 0.5023f, 
    0.5126f, 0.5229f, 0.5332f, 0.5435f, 0.5539f, 0.5643f, 0.5747f, 0.5851f, 0.5956f, 0.6061f, 0.6165f, 
    0.6270f, 0.6376f, 0.6481f, 0.6587f, 0.6693f, 0.6800f, 0.6907f, 0.7014f, 0.7121f, 0.7229f, 0.7338f, 
    0.7446f, 0.7555f, 0.7664f, 0.7774f, 0.7884f, 0.7994f, 0.8104f, 0.8215f, 0.8325f, 0.8436f, 0.8547f, 
    0.8658f, 0.8769f, 0.8880f, 0.8991f, 0.9102f, 0.9213f, 0.9325f, 0.9437f, 0.9548f, 0.9661f, 0.9773f, 
    0.9886f, 1.0000f
};

LookupTable_S socByOcvTable = { .length = TABLE_LENGTH, .x = openCellVoltage, .y = stateOfCharge};

LookupTable_S soeFromSocTable = { .length = TABLE_LENGTH, .x = stateOfCharge, .y = stateOfEnergy};


/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

/*!
  @brief   Get the state of charge (SOC) based on the cell voltage.
  @param   cellVoltage - The cell voltage to be used for the SOC lookup.
  @return  The state of charge as a percentage
*/
static float getSocFromCellVoltage(float cellVoltage)
{
    return lookup(cellVoltage, &socByOcvTable);
}

/*!
  @brief   Get the state of energy (SOE) based on the state of charge (SOC).
  @param   soc - The state of charge to be used for the SOE lookup.
  @return  The state of energy in percent
*/
static float getSoeFromSoc(float soc)
{
    return lookup(soc, &soeFromSocTable);
}

/*!
  @brief   Convert state of charge (SOC) to milliCoulombs.
  @param   soc - The state of charge as a fraction (0.0 to 1.0) to be converted.
  @return  The equivalent milliCoulombs for the provided state of charge
*/
static uint32_t getMilliCoulombsFromSoc(float soc)
{
    if (soc < 0.0f)
    {
        soc = 0.0f;
    }
    if (soc > 1.0f)
    {
        soc = 1.0f;
    }
    return (uint32_t)(MAX_ACCUMULATOR_MILLICOULOMBS * soc);
}

/*!
  @brief   Convert milliCoulombs to state of charge (SOC).
  @param   milliCoulombs - The milliCoulombs to be converted to state of charge.
  @return  The equivalent state of charge (fraction between 0.0 and 1.0) for the provided milliCoulombs.
*/
static float getSocFromMilliCoulombs(uint32_t milliCoulombs)
{
    if (milliCoulombs > MAX_ACCUMULATOR_MILLICOULOMBS)
    {
        milliCoulombs = MAX_ACCUMULATOR_MILLICOULOMBS;
    }
    return ((float)milliCoulombs)/((float)MAX_ACCUMULATOR_MILLICOULOMBS);
}

/*!
  @brief   Update the method used for calculating the state of charge (SOC) based on the accumulator current.
  @param   soc - Pointer to the Soc_S struct containing the accumulator current and other SOC-related information.
  
  This function determines if the SOC should be calculated using the open-circuit voltage (OCV) or Coulomb counting
  method based on the accumulator current. If the accumulator current is low enough, it updates a timer to check
  if the OCV method can be relied upon.
*/
static void updateSocMethod(Soc_S* soc)
{
    // Update the socByOcvGood timer to see if we should be using soc by ocv or coulomb counting
    if (fabsf(soc->curAccumulatorCurrent) < 3.0f)
    {
        // Accumulator current is low enough where the polarization of the cells should be decaying.
        // Update timer to see if we can rely on SOC by OCV
        updateTimer(&soc->socByOcvGoodTimer);
        if (checkTimerExpired(&soc->socByOcvGoodTimer))
        {
            // Indicates that the OCV has stabilized enough for a reliable SOC by OCV reading
            soc->socByOcvGood = true;
        }
    }
    else
    {
        // Accumulator current is too high to rely on SOC by OCV. Reset qualification timer
        clearTimer(&soc->socByOcvGoodTimer);
        if (soc->socByOcvGood)
        {
            // Save current SOC value as initial reference for coulomb counting and reset coulomb counter
            soc->socByOcvGood = false;
            soc->coulombCounter.accumulatedMilliCoulombs = 0;
            soc->coulombCounter.initialMilliCoulombCount = getMilliCoulombsFromSoc(soc->socByOcv);
        }
    }
}

/*!
  @brief   Update the Coulomb counter based on the accumulator current and elapsed time.
  @param   soc - Pointer to the Soc_S struct containing the accumulator current, elapsed time, and Coulomb counter information.
  
  This function calculates the change in milliCoulombs based on the accumulator current and elapsed time, and updates
  the accumulated milliCoulombs in the Coulomb counter.
*/
static void countCoulombs(Soc_S* soc)
{
    int32_t deltaMilliCoulombs =  soc->deltaTimeMs * soc->curAccumulatorCurrent;
    soc->coulombCounter.accumulatedMilliCoulombs += deltaMilliCoulombs;
}

/*!
  @brief   Calculate the state of charge (SOC) and state of energy (SOE) using the appropriate method.
  @param   soc - Pointer to the Soc_S struct containing the necessary information for SOC and SOE calculation.
  
  This function calculates the SOC and SOE using either the open-circuit voltage (OCV) method or the Coulomb counting
  method, depending on the current reliability of the OCV method.
*/
static void calculateSocAndSoe(Soc_S* soc)
{
    // Calculate current SOC either through SOC by OCV or through coulomb counting
    if (soc->socByOcvGood)
    {
        // Currently soc by ocv is a reliable estimate
        soc->socByOcv = getSocFromCellVoltage(soc->minBrickVoltage);
        soc->soeByOcv = getSoeFromSoc(soc->socByOcv);
        soc->socByCoulombCounting = 0;
        soc->soeByCoulombCounting = 0;
    }
    else
    {
        // Currently soc by ocv is unreliable. Do coulomb counting
        soc->socByOcv = getSocFromCellVoltage(soc->minBrickVoltage);
        soc->soeByOcv = getSoeFromSoc(soc->socByOcv);

        // Update coulomb counter
        const uint32_t currentMilliCoulombs = soc->coulombCounter.initialMilliCoulombCount + soc->coulombCounter.accumulatedMilliCoulombs;
        soc->socByCoulombCounting = getSocFromMilliCoulombs(currentMilliCoulombs);
        soc->soeByCoulombCounting = getSoeFromSoc(soc->socByCoulombCounting);
    }
}

/*!
  @brief   Update the state of charge (SOC) and state of energy (SOE) using the appropriate method.
  @param   soc - Pointer to the Soc_S struct containing the necessary information for SOC and SOE calculation.
  
  This function first updates the SOC calculation method based on the accumulator current, then updates the Coulomb
  counter if needed, and finally calculates the SOC and SOE using the appropriate method.
*/
void updateSocAndSoe(Soc_S* soc)
{
    updateSocMethod(soc);

    if (!soc->socByOcvGood)
    {
        countCoulombs(soc);
    }

    calculateSocAndSoe(soc);
}

