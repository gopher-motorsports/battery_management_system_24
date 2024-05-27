#ifndef INC_TIMER_H_
#define INC_TIMER_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t timCount;
    uint32_t lastUpdate;
    uint32_t timThreshold;
} Timer_S;

/*!
    @brief   Updates the given timer with the elapsed time since the last update
    @param   timer - Pointer to the Timer_S structure to be updated
*/
void updateTimer(Timer_S* timer);

/*!
    @brief   Configures a timer with a given threshold and reset timer count
    @param   timer - Pointer to the Timer_S structure to configure
    @param   timerThreshold - The timer threshold to set
*/
void configureTimer(Timer_S* timer, uint32_t timerThreshold);

/*!
    @brief   Clears the count of a timer
    @param   timer - Pointer to the timer to be cleared.
*/
void clearTimer(Timer_S* timer);

/*!
    @brief Set the timer count to its maximum value (saturation)
    @param timer - Pointer to the Timer_S struct to be saturated
*/
void saturateTimer(Timer_S* timer);

/*!
  @brief   Get the time remaining until the timer expires.
  @param   timer - Pointer to the Timer_S struct containing the timer's threshold and count values.
  @return  The time remaining in milliseconds until the timer expires.
*/
uint32_t getTimeTilExpirationMs(Timer_S* timer);

/*!
    @brief   Check whether or not the timer has reached its threshold (expired)
    @param   timer - Pointer to the Timer_S struct to be checked
    @returns True if timer expired, false otherwise
*/
bool checkTimerExpired(Timer_S* timer);


#endif /* INC_TIMER_H_ */
