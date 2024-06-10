/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <string.h>
#include "bmbUtils.h"
#include "lookupTable.h"


/* ==================================================================== */
/* ============================= DEFINES ============================== */
/* ==================================================================== */



/* ==================================================================== */
/* ========================= LOCAL VARIABLES ========================== */
/* ==================================================================== */

/* ==================================================================== */
/* ======================== GLOBAL VARIABLES ========================== */
/* ==================================================================== */


/* ==================================================================== */
/* =================== LOCAL FUNCTION DEFINITIONS ===================== */
/* ==================================================================== */

/*!
  @brief   Search a sorted array of Brick_S structs using binary search
  @param   arr - Pointer to the Brick_S array to be searched
  @param   l - Index of the left element of the array 
  @param   r - Index of the right element of the array
  @param   v - The target voltage to be compared to the Brick_S struct voltage
  @return  Index at which voltage is to be inserted
*/
int32_t brickBinarySearch(Brick_S *arr, int l, int r, float v)
{
  while (l <= r)
  {
    int32_t m = l + (r - l) / 2;
    if (fequals(arr[m].brickV, v))
    {
    	return m;
    }
    if (arr[m].brickV < v)
    {
    	l = m + 1;
    }
    else
    {
    	r = m - 1;
    }
  }
  return l;
}


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DEFINITIONS ==================== */
/* ==================================================================== */

/*!
  @brief   Sorts an array of Brick_S structs by their voltage from lowest to highest
  @param   arr - Pointer to the Brick_S array to be sorted
  @param   numBricks - The length of the array to be sorted
*/
void insertionSort(Brick_S *arr, int numBricks)
{
  for (int32_t unsortedIdx = 1; unsortedIdx < numBricks; unsortedIdx++)
  {
    Brick_S temp = arr[unsortedIdx];
    int32_t sortedIdx = unsortedIdx - 1;
    int32_t pos = brickBinarySearch(arr, 0, sortedIdx, temp.brickV);
    while (sortedIdx >= pos)
    {
      arr[sortedIdx + 1] = arr[sortedIdx];
      sortedIdx--;
    }
    arr[sortedIdx + 1] = temp;
  }
}
