#ifndef  _H_COMPAT_
#define _H_COMPAT_

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>


// For Microsoft Visual Studio
#ifdef _MSC_VER

typedef signed char int8_t; 
typedef signed short int16_t; 
typedef signed long int32_t; 
typedef unsigned char uint8_t; 
typedef unsigned short uint16_t; 
typedef unsigned long uint32_t; 
typedef unsigned long long uint64_t;

#ifndef log2                          // no log2 in xp
#define log2(x) (log(x)/log(2))       // there are fast tricks for finding int(log2(x))
#endif                             

#define inline __inline
#define msvcextern extern

inline long int lround(float x);
inline double round(double x);
inline float roundf(float x);

#define isnan _isnan

#else  // NOT MSVC

#include <stdint.h>
#define msvcextern 
#ifndef __declspec
#define __declspec(x) 
#endif
#endif

#ifndef INFINITY
  #define INFINITY FLT_MAX
#endif

#define SHARED_EXPORT __declspec(dllexport)

#endif //#define _H_COMPAT_
