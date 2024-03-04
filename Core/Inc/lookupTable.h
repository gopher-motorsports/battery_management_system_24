#ifndef INC_LOOKUPTABLE_H_
#define INC_LOOKUPTABLE_H_

/* ==================================================================== */
/* ============================= INCLUDES ============================= */
/* ==================================================================== */

#include <stdint.h>


/* ==================================================================== */
/* ============================== STRUCTS============================== */
/* ==================================================================== */

//Generic Lookup Table
typedef struct
{
    const uint32_t length;
    const float* x;			// Pointer to x array
    const float* y;			// Pointer to y array
} LookupTable_S;


float lookup(float x, const LookupTable_S* table);


#endif /* INC_LOOKUPTABLE_H_ */
