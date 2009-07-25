#ifndef  _H_COMPAT_
#define _H_COMPAT_

#include <float.h>

// For Microsoft Visual Studio
#ifdef _MSC_VER
typedef signed char int8_t; 
typedef signed short int16_t; 
typedef signed long int32_t; 
typedef unsigned char uint8_t; 
typedef unsigned short uint16_t; 
typedef unsigned long uint32_t; 

#ifndef log2
#define log2(x) (log(x)/log(2))
#endif

#define inline __inline
#define msvcextern extern

inline long int lround(float x);
inline double round(double x);
inline float roundf(float x);
inline char* rindex(char *,char);

#define isnan _isnan

#else 
#include <stdint.h>
#define msvcextern 
#endif

#ifndef INFINITY
  #define INFINITY FLT_MAX
#endif


#endif //#define _H_COMPAT_
