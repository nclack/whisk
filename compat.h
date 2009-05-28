#ifndef  _H_COMPAT_
#define _H_COMPAT_

#include <float.h>

// For Microsoft Visual Studio
#ifdef _MSC_VER
#define inline __inline
#define msvcextern extern

inline long int lround(float x);
inline double round(double x);
inline float roundf(float x);

#define isnan _isnan

#else 
#define msvcextern 
#endif

#ifndef INFINITY
  #define INFINITY FLT_MAX
#endif


#endif //#define _H_COMPAT_
