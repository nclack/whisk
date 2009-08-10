#include "utilities.h"
#include "deque.h"
#include <assert.h>

#if 0
#define DEBUG_REQUEST_STORAGE
#endif

void *request_storage( void *buffer, size_t *maxlen, size_t nbytes, size_t minindex, char *msg )
{ if( (nbytes*minindex) > *maxlen )
  {
#ifdef DEBUG_REQUEST_STORAGE
    printf("REQUEST %7d bytes (%7d items) above current %7d bytes by %s\n",minindex * nbytes, minindex, *maxlen,msg);
#endif
    *maxlen = (size_t) (1.25 * minindex * nbytes + 64);
    buffer = Guarded_Realloc( buffer, *maxlen, msg );
  }
  return buffer;
}

/**
 * Based on:
 * Daniel Lemire, "Streaming Maximum-minimum filter using no more than
 * three comparisons per element', Nordic Journal of Computing, 13(4) pp. 328-339, 2006.
 *
 */
void maxfilt_double_inplace( double *a, int len, int support )
{ static Deque *U = NULL;
  double *e;
  int off = support;
  assert(support > 2);

  if( !U )
    U = Deque_Alloc( support );
  else
    Deque_Reset(U);

  for( e=a+1; e < a+len; e++ )
  { if( e >= a+off )
      e[-off] = Deque_Is_Empty(U) ? e[-1] : *(double*)Deque_Front(U); // YIELD 

    if( e[0] > e[-1] )
    { while( !Deque_Is_Empty(U) )
      { if( e[0] <= *(double*)Deque_Pop_Back(U) )
        { if( e == support + (double*)Deque_Front(U) )
            Deque_Pop_Front(U);
          break;
        }
        Deque_Pop_Back(U);
      } 
    } else // then e[0] <= e[-1]
    { Deque_Push_Back(U, e-1);
      if( e == support + (double*)Deque_Front(U) )
        Deque_Pop_Front(U);
    }
  }
  a[len-off] = Deque_Is_Empty(U) ? e[-1] : *(double*)Deque_Front(U);
}

void maxfilt_centered_double_inplace( double *a, int len, int support )
{ static Deque *U = NULL,
               *R = NULL;
  double *e, v;
  int off = support;
  int c = 0;
  assert(support > 2);

  if( !U )
  { U = Deque_Alloc( support );
    R = Deque_Alloc( c+1 );
  } else
  { Deque_Reset(U);
    Deque_Reset(R);
  }

  //The beginning

  //The middle (support entirely in interval)
  for( e=a+1; e < a+len; e++ )
  { if( e-a >= off )
      Deque_Push_Front(R, Deque_Is_Empty(U) ? e-1 : Deque_Front(U) ); // YIELD  
    if( e-a >= off + c ) 
      e[-off] = *(double*)Deque_Pop_Back(R);                          // STORE
     
    if( e[0] > e[-1] )
    { while( !Deque_Is_Empty(U) )
      { if( e[0] <= *(double*)Deque_Pop_Back(U) )
        { if( e-off == Deque_Front(U) )
            Deque_Pop_Front(U);
          break;
        }
        Deque_Pop_Back(U);
      } 
    } else // then e[0] <= e[-1]
    { Deque_Push_Back(U, e-1);
      if( e-off == Deque_Front(U) )
        Deque_Pop_Front(U);
    }
  }
  //The end
  for( ; e < a+len+off; e++ )
  { Deque_Push_Front(R, Deque_Is_Empty(U) ? e-1 : Deque_Front(U) ); // YIELD  
    e[-off] = *(double*)Deque_Pop_Back(R);                          // STORE

    Deque_Push_Back(U, e-1); 
    if( e-off == Deque_Front(U) )
      Deque_Pop_Front(U);
  }

}

#ifdef TEST_MAX_FILT_1
double testdata[] = {-1.0, 2.0,-3.0, 4.0, 
                      3.0, 2.0, 1.0, 0.0,
                      1.0, 2.0, 3.0, 4.0,
                      3.0, 2.0, 1.0, 4.0};
double expected[] = { 2.0, 4.0, 4.0, 4.0, 
                      3.0, 2.0, 1.0, 2.0,
                      3.0, 4.0, 4.0, 4.0,
                      3.0, 4.0, 4.0, 4.0};
//double expected[] = { 2.0, 2.0, 4.0, 4.0, 
//                      4.0, 3.0, 2.0, 1.0,
//                      2.0, 3.0, 4.0, 4.0,
//                      4.0, 3.0, 2.0, 1.0};
#endif
#ifdef TEST_MAX_FILT_2
double testdata[] = { 1.0, 0.0, 0.0, 0.0, 
                      0.0, 0.0, 1.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 1.0};
double expected[] = { 1.0, 0.0, 0.0, 0.0, 
                      1.0, 1.0, 1.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 1.0, 1.0, 1.0};
#endif
                    

#if defined(TEST_MAX_FILT_1) || defined(TEST_MAX_FILT_2)
int main(int argc, char* argv[])
{ int i;
  for(i=0;i<16;i++)
    progress("%2d %f\n",i,testdata[i]);
  maxfilt_centered_double_inplace( testdata, 16, 3 );
  progress("\n");
  for(i=0;i<16;i++)
    progress("%2d %3.2f %3.2f %d\n",i,testdata[i],expected[i],testdata[i]==expected[i]);
  return 0;
}

#endif
