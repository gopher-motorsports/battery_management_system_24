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
    3.002f, 3.135f, 3.223f, 3.293f, 3.343f, 3.383f, 3.405f, 3.42f, 3.427f, 3.435f, 3.442f, 3.449f, 3.457f, 
    3.464f, 3.473f, 3.485f, 3.492f, 3.5f, 3.508f, 3.515f, 3.522f, 3.53f, 3.537f, 3.543f, 3.544f, 3.552f, 3.559f, 
    3.565f, 3.566f, 3.574f, 3.577f, 3.581f, 3.587f, 3.588f, 3.596f, 3.596f, 3.603f, 3.603f, 3.609f, 3.615f, 3.617f, 
    3.624f, 3.625f, 3.631f, 3.632f, 3.639f, 3.646f, 3.647f, 3.654f, 3.661f, 3.666f, 3.673f, 3.676f, 3.683f, 3.691f, 
    3.701f, 3.712f, 3.719f, 3.728f, 3.741f, 3.749f, 3.763f, 3.771f, 3.786f, 3.796f, 3.807f, 3.821f, 3.829f, 3.844f, 
    3.852f, 3.866f, 3.877f, 3.888f, 3.9f, 3.91f, 3.923f, 3.932f, 3.945f, 3.954f, 3.968f, 3.981f, 3.99f, 4.004f, 4.014f,
    4.027f, 4.04f, 4.054f, 4.063f, 4.078f, 4.092f, 4.101f, 4.114f, 4.129f, 4.143f, 4.155f, 4.166f, 4.18f, 4.195f, 4.209f, 
    4.23f, 4.267
};

// State of energy for VTC6 cells. References SOC table
float stateOfEnergy[TABLE_LENGTH] = 
{
    0.00f, 0.01f, 0.02f, 0.03f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.08f, 0.09f, 0.10f, 0.11f, 
    0.12f, 0.13f, 0.14f, 0.15f, 0.15f, 0.16f, 0.17f, 0.18f, 0.19f, 0.20f, 0.21f, 0.22f, 0.23f, 
    0.24f, 0.25f, 0.26f, 0.27f, 0.28f, 0.29f, 0.30f, 0.31f, 0.32f, 0.33f, 0.34f, 0.35f, 0.36f, 
    0.37f, 0.37f, 0.38f, 0.39f, 0.40f, 0.41f, 0.42f, 0.43f, 0.44f, 0.45f, 0.46f, 0.47f, 0.48f, 
    0.49f, 0.50f, 0.51f, 0.52f, 0.53f, 0.54f, 0.55f, 0.56f, 0.57f, 0.58f, 0.59f, 0.60f, 0.61f, 
    0.62f, 0.63f, 0.64f, 0.65f, 0.66f, 0.67f, 0.68f, 0.70f, 0.71f, 0.72f, 0.73f, 0.74f, 0.75f, 
    0.76f, 0.77f, 0.78f, 0.79f, 0.80f, 0.81f, 0.82f, 0.83f, 0.84f, 0.86f, 0.87f, 0.88f, 0.89f, 
    0.90f, 0.91f, 0.92f, 0.93f, 0.94f, 0.95f, 0.97f, 0.98f, 0.99f, 1.00
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

