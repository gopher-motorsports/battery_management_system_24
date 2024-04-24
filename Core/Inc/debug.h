#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

#define DEBUG 0
#if DEBUG
	#define Debug(__info,...) printf("Debug: " __info,##__VA_ARGS__)
#else
	#define Debug(__info,...)  
#endif

#define DEBUG_IMD 0
#if DEBUG_IMD
	#define DebugIMD(__info,...) printf("Debug: " __info,##__VA_ARGS__)
#else
	#define DebugIMD(__info,...)  
#endif

#endif