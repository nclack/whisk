#ifndef H_AIP
#define H_AIP

#include <stdio.h>
typedef float real; // could be double.
#define C const
#define bigReal 1.E38f  // FLT_MAX, DBL_MAX if double above
#include <limits.h>

typedef struct _point {real x; real y;}                       point;
typedef struct _box   {point min; point max;}                 box;
typedef long long                                             hp;
typedef struct ipoint {long x; long y;}                       ipoint;
typedef struct rng    {long mn; long mx;}                     rng;
typedef struct vertex {ipoint ip; rng rx; rng ry; short in;}  vertex;

real inter(point * a, int na, point * b, int nb);

#endif // H_AIP
