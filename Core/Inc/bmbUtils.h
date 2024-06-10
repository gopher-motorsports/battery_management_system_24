#ifndef INC_BMBUTILS_H_
#define INC_BMBUTILS_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>


/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */


typedef struct
{
	int brickIdx;
	float brickV;
} Brick_S;


/* ==================================================================== */
/* ======================= EXTERNAL VARIABLES ========================= */
/* ==================================================================== */


/* ==================================================================== */
/* =================== GLOBAL FUNCTION DECLARATIONS =================== */
/* ==================================================================== */

/*!
  @brief   Sorts an array of Brick_S structs by their voltage from lowest to highest
  @param   arr - Pointer to the Brick_S array to be sorted
  @param   numBricks - The length of the array to be sorted
*/
void insertionSort(Brick_S *arr, int numBricks);

#endif /* INC_BMBUTILS_H_ */
