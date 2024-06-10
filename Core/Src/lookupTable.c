/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include "lookupTable.h"
#include <stdbool.h>
#include <math.h>
#include "utils.h"


/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */

// The max number of calls that are allowed in a binary search instance before erroring
#define MAX_DEPTH 20


/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

/*!
    @brief   Search in an array of floats for a specific value using binary search
    @param   arr - The array of floats to search
    @param   target - The target value to search for
    @param   low - The lowest index in the search range
    @param   high - The highest index in the search range
    @return  The index of the upper float that bounds the value of interest
*/
static uint32_t binarySearch(const float *arr, const float target, int low, int high)
{
    //check for invalid bounds
    if(low > high)
    {
        return 0;
    }

    //clamp target to bounds
    if(target < arr[low])
    {
        return low;
    }
    else if(target > arr[high])
    {
        return high;
    }

    uint8_t depth = 0;
    while((low <= high) && (depth < MAX_DEPTH))
    {
        //update middle value
        int mid = (high - low) / 2 + low;

        //target is below index m
        if(target <= arr[mid+1])
        {
            //target is between index m and m-1, return m
            if(target >= arr[mid])
            {
                return mid;
            }
            //target is less than index m, update bounds
            else
            {
                high = mid;
            }
        }
        //target is greater than index m, update bounds
        else
        {
            low = mid + 1;
        }
        depth++;
    }
    return 0;
}

/*!
    @brief   Interpolate a value y given two endpoints (x1, y1) and (x2, y2) and an input value x
    @param   x - The input value to interpolate
    @param   x1 - The x-coordinate of the first endpoint
    @param   x2 - The x-coordinate of the second endpoint
    @param   y1 - The y-coordinate of the first endpoint
    @param   y2 - The y-coordinate of the second endpoint
    @return  The interpolated value of y at x
*/
static float interpolate(float x, float x1, float x2, float y1, float y2)
{
    //if infinite slope, return y2
    if (fequals(x1, x2))
    {
        return y2;
    }
    else
    {
        // map input x to y axis with slope m = (y2-y1)/(x2-x1)
        return (x - x1) * ((y2-y1) / (x2-x1)) + y1;
    }
}

/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */
/*!
    @brief   Look up the value of a function in a lookup table, given an input value
    @param   x - The input value to look up in the table
    @param   table - A pointer to the lookup table containing the function values
    @return  The interpolated value of the function corresponding to the input value, or 0 if an error occurs
*/
float lookup(float x, const LookupTable_S* table)
{
    //Clamp input to bounds of lookup table
    if(x < table->x[0])
    {
        return table->y[0];
    }
    else if(x > table->x[table->length-1])
    {
        return table->y[table->length-1];
    }

    //Search for index of lookup table voltage that is closest to and less than input voltage
    uint32_t i = binarySearch(table->x, x, 0, table->length-1);

    //Return 0 if binary search error
    if(i == -1)
    {
        return 0;
    }

    //Interpolate temperature from lookup table
    return interpolate(x, table->x[i], table->x[i+1], table->y[i], table->y[i+1]);
}
