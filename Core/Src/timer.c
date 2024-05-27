/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include "main.h"
#include "timer.h"

/*!
    @brief   Updates the given timer with the elapsed time since the last update
    @param   timer - Pointer to the Timer_S structure to be updated
*/
void updateTimer(Timer_S* timer)
{
    uint32_t timeTilExpiration = timer->timThreshold - timer->timCount;
    uint32_t timeSinceLastUpdate = HAL_GetTick() - timer->lastUpdate;
    timer->lastUpdate = HAL_GetTick();
    if (timeTilExpiration < timeSinceLastUpdate)
    {
        timer->timCount += timeTilExpiration;
    }
    else
    {
        timer->timCount += timeSinceLastUpdate;
    }
}

/*!
    @brief   Configures a timer with a given threshold and reset timer count
    @param   timer - Pointer to the Timer_S structure to configure
    @param   timerThreshold - The timer threshold to set
*/
void configureTimer(Timer_S* timer, uint32_t timerThreshold)
{
    timer->timCount = 0;
    timer->lastUpdate = HAL_GetTick();
    timer->timThreshold = timerThreshold;
}

/*!
    @brief   Clears the count of a timer
    @param   timer - Pointer to the timer to be cleared.
*/
void clearTimer(Timer_S* timer)
{
    timer->timCount = 0;
    timer->lastUpdate = HAL_GetTick();
}

/*!
    @brief Set the timer count to its maximum value (saturation)
    @param timer - Pointer to the Timer_S struct to be saturated
*/
void saturateTimer(Timer_S* timer)
{
    timer->timCount = timer->timThreshold;
}

/*!
  @brief   Get the time remaining until the timer expires.
  @param   timer - Pointer to the Timer_S struct containing the timer's threshold and count values.
  @return  The time remaining in milliseconds until the timer expires.
*/
uint32_t getTimeTilExpirationMs(Timer_S* timer)
{
    return timer->timThreshold - timer->timCount;
}

/*!
    @brief   Check whether or not the timer has reached its threshold (expired)
    @param   timer - Pointer to the Timer_S struct to be checked
    @returns True if timer expired, false otherwise
*/
bool checkTimerExpired(Timer_S* timer)
{
    if (timer->timCount >= timer->timThreshold)
    {
        return true;
    }
    else
    {
        return false;
    }
}