#include <assert.h>
#include <stdio.h>
#include "utilities.h"
#include "common.h"

//
// Matrix Algrebra
//
// Many of the functions defined here overlap with well known
// and strongly optimized available linear algebra packages.
//
// The reasone they are reimplimented here is so that 
// (a) I could have basic operations in the absence of an
//     external library.
// (b) To learn.
//
// These are not necessarily very fast.
// Matrix elements are assumed to be in row-major order
//

// Macro: generates a wrapper function where storage
//        for the result is statically allocated as a
//        resizable vector/matrix.
// See examples below for use of this macro.
// TODO: calling conventions are pretty uniform
//       using these might simplify the macros
//
#define MATMUL_CREATE_STATIC_WRAPPER_HEAD(name, ...) \
double* name##_static( __VA_ARGS__ )                           

#define MATMUL_CREATE_STATIC_WRAPPER_BODY(name, nelem, ...)               \
  static double *dest = NULL;                                             \
  static size_t dest_size = 0;                                            \
  dest = request_storage( dest, &dest_size,                               \
                          sizeof(double), (nelem),                        \
                          "Alloc for static matrix multiplication" );     \
  name( __VA_ARGS__ , dest );                                             \
  return dest
 

////////////////////////////////
// ......................     //
// INPUT/OUTPUT/DEBUGGING     //
// ......................     //
////////////////////////////////

void mat_print( double *M, int nrows, int ncols )
{ int i,j;
  for(i=0;i<nrows;i++)
  { for(j=0; j<ncols; j++)
      printf("% 5.5g   ",M[i*ncols+j]);
    printf("\n");
  }
}

////////////////////////////////
// .........                  //
// UTILITIES                  //
// .........                  //
////////////////////////////////
double **mat_index( double *M, int nrows, int ncols )
{ double **i = Guarded_Malloc( nrows*sizeof(double*), "matrix index");
  while(nrows--)
    i[nrows] = M + nrows*ncols;
  return i;
}

////////////////////////////////
// ..............             //
// MULTIPLICATION             //
// ..............             //
// /////////////////////////////

// A x B in O( nar * nac * nbc )
// TODO: test
void matmul( double *a, int nar, int nac, double *b, int nbr, int nbc, double *dest )
{ int i,j,k;
  double *rowa, *colb, *rowdest;
  assert(nac==nbr);
  for(i=0; i<nar; i++)
  { rowa    = a    + i*nac;
    rowdest = dest + i*nbc; 
    for( k=0; k<nbc; k++ )
    { double acc = 0.0;
      colb = b + k;
      for( j=0; j<nac; j++ )
        acc += rowa[j] * colb[nbc*j];
      rowdest[k] = acc;
    }
  }
}

MATMUL_CREATE_STATIC_WRAPPER_HEAD(matmul, double* a, int nar, int nac, double* b, int nbr, int nbc)
{ 
  MATMUL_CREATE_STATIC_WRAPPER_BODY(matmul, nac*nbc, a, nar, nac, b, nbr, nbc);
}

// A x transpose(B) in O( nar * nac * nbc ) 
// TODO: test
void matmul_right_transpose( double *a, int nar, int nac, double *b, int nbr, int nbc, double *dest )
{ int i,j,k;
  double *rowa, *colb, *rowdest;
  assert(nac==nbc);
  for(i=0; i<nar; i++)
  { rowa    = a    + i*nac;
    rowdest = dest + i*nbc; 
    for( k=0; k<nbr; k++ )
    { double acc = 0.0;
      colb = b + k*nbc;
      for( j=0; j<nac; j++ )
        acc += rowa[j] * colb[j];
      rowdest[k] = acc;
    }
  }
}

MATMUL_CREATE_STATIC_WRAPPER_HEAD(matmul_right_transpose, 
                                  double* a, int nar, int nac, 
                                  double* b, int nbr, int nbc)
{ MATMUL_CREATE_STATIC_WRAPPER_BODY(matmul_right_transpose, 
                                    nac*nbc, 
                                    a, nar, nac, 
                                    b, nbr, nbc);
}

// transpose(A) x B in O( nar * nac * nbc ) 
// TODO: test
void matmul_left_transpose( double *a, int nar, int nac, double *b, int nbr, int nbc, double *dest )
{ int i,j,k;
  double *rowa, *colb, *rowdest;
  assert(nar==nbr);
  for(i=0; i<nac; i++)
  { rowa    = a    + i;
    rowdest = dest + i*nbc; 
    for( k=0; k<nbc; k++ )
    { double acc = 0.0;
      colb = b + k;
      for( j=0; j<nar; j++ )
        acc += rowa[j*nac] * colb[nbc*j];
      rowdest[k] = acc;
    }
  }
}
MATMUL_CREATE_STATIC_WRAPPER_HEAD(matmul_left_transpose, 
                                  double* a, int nar, int nac, 
                                  double* b, int nbr, int nbc)
{ MATMUL_CREATE_STATIC_WRAPPER_BODY(matmul_left_transpose, 
                                    nac*nbc, 
                                    a, nar, nac, 
                                    b, nbr, nbc);
}

// diag(vec) * mat in O(nrows*ncols)
// dest must be nrows*ncols elements
// TODO: test
//
void matmul_left_vec_as_diag( double *vec, int n_vec, double *mat, int nrows, int ncols, double *dest )
{ int i=n_vec;
  double *row, v, *drow;
  assert( n_vec == nrows );
  while(i--)
  { int j   = ncols, 
        off = ncols*i;
    row  = mat  + off;
    drow = dest + off; 
    v = vec[i];
    while(j--)
      drow[j] = v * row[j];
  }
}
MATMUL_CREATE_STATIC_WRAPPER_HEAD(matmul_left_vec_as_diag, 
                                  double* a, int nac,
                                  double* b, int nbr, int nbc)
{ MATMUL_CREATE_STATIC_WRAPPER_BODY(matmul_left_vec_as_diag,
                                    nac*nbc, 
                                    a, nac,
                                    b, nbr, nbc);
}

// mat*diag(vec) in O(nrows*ncols)
// dest must be nrows*ncols elements
// TODO: test
//
void matmul_right_vec_as_diag( double *mat, int nrows, int ncols, double *vec, int n_vec, double *dest )
{ int i=nrows;
  double *row, *drow;
  assert( n_vec == ncols );
  while(i--)
  { int j   = ncols, 
        off = ncols*i;
    row  = mat  + off;
    drow = dest + off; 
    while(j--)
      drow[j] = vec[j] * row[j];
  }  
}
MATMUL_CREATE_STATIC_WRAPPER_HEAD(matmul_right_vec_as_diag,
                                  double* a, int nar, int nac,
                                  double* b, int nbc)
{ MATMUL_CREATE_STATIC_WRAPPER_BODY(matmul_right_vec_as_diag,
                                    nac*nbc, 
                                    a, nar, nac,
                                    b, nbc);
}
