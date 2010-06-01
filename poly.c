#include "compat.h"
#include <string.h>
#include "common.h"
#include "utilities.h"
#include <math.h>
#include <assert.h>
#include "mat.h"
#include "svd.h"
#include "poly.h"

#define POLYFIT_SINGULAR_THRESHOLD (1e-6)

#if 0
#define DEBUG_POLYFIT
#define DEBUG_POLYADD_IP 
#define DEBUG_POLYFIT_REUSE
#endif

inline double polyval(double *p, int degree, double x)
{ double acc  = 0.0,
         xton = 1.0,
         *end = p + degree + 1; // #coeffs = degree + 1
  //coefficient for smallest value is first
  while(p < end)
  { acc  += xton * (*p++);
    xton *= x;
  }
  return acc;
}

inline int polymul_nelem_dest(int na, int nb)
{ return na + nb - 1;
}

//
// Multiplies polynomials by convolution.  Expects destination to be
// pre-allocated with na+nb+1 # of elements (see: polymul_nelem_dest).
//
inline void polymul(double *a, int na, double *b, int nb, double* dest )
{ int j,k;
  double *aend = a + na;
  nb -= 1; // shift this to index of last element for convenience
  k = polymul_nelem_dest(na,nb);
  memset( dest,0, sizeof(double)*k );
  while(k--)
  { double acc = 0.0,
           *bkj = b + ((k<nb) ? k : nb), // b[k-j] start
           *aj  = a + k - (bkj - b);   // a[j]   start
    if( aj < a ) continue;
    while( ( bkj >= b ) && (aj < aend ) )
      acc += (*bkj--)*(*aj++);
    dest[k] = acc; 
  }
}

//
// In place addition of two polynomials.
// Requires na > nb.
//
inline void polyadd_ip_left(double *a, int na, double *b, int nb )
{ 
#ifdef DEBUG_POLYADD_IP
  assert(na>nb);
#endif
  while( nb-- )
    a[nb] += b[nb];
}

inline void polysub_ip_left(double *a, int na, double *b, int nb )
{ 
#ifdef DEBUG_POLYADD_IP
  assert(na>nb);
#endif
  while( nb-- )
    a[nb] -= b[nb];
}

inline void polyadd ( double *a, int na, double *b, int nb, double *dest )
{ 
  while( na > nb )
    dest[--na] = a[na];
  while( nb > na )
    dest[--nb] = b[nb];
  assert(na==nb);
  while( na-- )
    dest[na] = a[na] + b[na];
}

inline void polysub ( double *a, int na, double *b, int nb, double *dest )
{ 
  while( na > nb )
    dest[--na] = a[na];
  while( nb > na )
    dest[--nb] = b[nb];
  assert(na==nb);
  while( na-- )
    dest[na] = a[na] - b[na];
}
//
// Derivative of polynomial
//
inline void polyder_ip( double *a, int na, int times )
{ int i;
  if(times>0)
  { for(i=1;i<na;i++)
      a[i-1] = i * a[i];
    a[na-1] = 0.0;
    polyder_ip( a, na-1, times-1 );
  }
}

//
// allocate your own storage for V
//
// TODO: TEST
//

void Vandermonde_Build( double *x, int n, int degree, double *V )
{ int i,j;
  for( i=0; i<n; i++ )
  { double v = x[i],
         acc = 1.0,
        *row = V + i * degree;
    for( j=0; j<degree; j++ )
    { row[j] = acc;
      acc *= v;
    }
  }
}

//
// `invV` must be an n*n array of doubles.
//
// Uses Parker-Traub algorithm to compute the inverse of the Vandermonde
// matrix given the generating points `x`.  Runs in O(n^2) time and space
// and is more numerically stable than Gauss-Jordon elimination.
//
// Follows:
// [1]  Gohberg and Olshevsky. The fast generalized Parker-Traub algorithm for
//      inversion of Vandermonde and related matrices. J Complexity (1997) 
//      vol. 13 (2) pp. 208-234 
//
//  TODO: UNTESTED
//  XXX : This doesn't look useful for polynomial fitting, though it's good for
//        interpolation. Might be better for numerical derivatives and
//        predictor-corrector algorithms, etc...  The main contribution of the
//        reference above is an inversion algorithm for Vandermonde-like
//        matrices, which (I think) includes minors of the nxn Vandermonde.
//        In the end, you're still limited by how the final multiply scales
//        [[ O(npoints * ndeg^2) ]], but it'll be a bit faster and according
//        to the paper it'll be more numerically _stable_.
//  TODO: test to see if taking doing q[i,j]/P'[x[j]] in logspace
//        is more numerically stable
//
void Vandermonde_Inverse( double *x, int n, double *invV)
{ static double *master = NULL;
  static size_t  master_size = 0;
  double *Q;

  // Compute master polynomial 
  // via nested polynomial multiplication ( eq 1.10 in [1] )
  // The resulting polynomial will be on the first row
  { int i,j;
    int stride = n;
    double *row,*last;
    double *master = invV; 
    //master = request_storage( master, &master_size, sizeof(double), n*n, "inverse vandermonde" );
    memset(master,0,sizeof(double)*n*n);
    
    master[(n-1)*stride]     = -x[0];
    master[(n-1)*stride + 1] =  1.0;
    last = master + (n-1)*stride;
    for(i=1;i<n-1;i++)                     // do n-1 rows
    { double v = x[i];
      row  = master + (n-i-1)*n;           // walk rows backwards
      row[0] -= v*last[0];                 // first column special
      for(j=1; j<i+2; j++)                 // last column gets done right //max j = max(i+1) = n-2+1 = n - 1
        row[j] = last[j-1] - v * last[j];  //    due to memset 0 above
      last = row;
    }
  }
  //
  // Compute the quotient coefficients according to [1] eq. 2.5
  // use the same space as for the master polynomial
  //
  { int stride = n;
    int i,j = stride;
    double *last;
    Q = master + stride; // start of quotient matrix
    while(j--) // initialize
      Q[j] = 1.0;
    last = Q;
    for( i=1; i<n-1; i++ )
    { double *row = Q + i*stride;
      double v = master[n-i];
      for( j=0; j<n; j++)
        row[j] = x[j] * last[j] + v;
      last = row;
    }
  }

  // 
  // Compute master'(x[j]) for all j ( eq 2.4 from [1] )
  // Store this in the first row of invV
  //
  { int i,j;
    for( i=0; i<n; i++ )
    { double acc = 1.0,
             v = x[i];
      for( j=0; j<i; j++ )
        acc *= ( v - x[j] ); 
      for( j=i+1; j<n; j++ )
        acc *= ( v - x[j] ); 
      invV[i] = acc;
    }
  }

  //
  // Compute invV
  //
  { int i,j;
    int stride = n;
    for( j=0; j<n; j++ ) // columns
    { double pp = invV[j],
           *col = invV + j;
      for( i=0; i<n; i++ ) // rows
        col[stride*i] = Q[ i*stride + j ] / pp;
    }
  }

}

//
// Vandermonde matrix determinant 
//
double Vandermonde_Determinant( double *x, int n )
{ int i,j;
  double acc = 1.0;
  for( i=0; i<n; i++ )
  { double t = x[i];
    for( j=i+1; j<n; j++ )
      acc *= (x[j] - t);
  }
  return acc;
}

//
// Vandermonde matrix determinant (Log2)
//
double Vandermonde_Determinant_Log2( double *x, int n )
{ int i,j;
  double acc = 0.0;
  for( i=0; i<n; i++ )
  { double t = x[i];
    for( j=i+1; j<n; j++ )
      acc += log2( x[j] - t );
  }
  return acc;
}

//
// POLYFIT
//

inline int polyfit_size_workspace(int n, int degree )
{ int ncoeffs = degree+1;
  return (n + 1 + ncoeffs)*ncoeffs;
}

double *polyfit_alloc_workspace( int n, int degree )
{ degree += 1; // e.g. for degree 2 need 3 coefficients
  return Guarded_Malloc( sizeof(double)*( polyfit_size_workspace(n,degree) ), "polyfit workspace" );
}

void polyfit_realloc_workspace( int n, int degree, double **workspace )
{ degree += 1; // e.g. for degree 2 need 3 coefficients
  if(workspace)
  { *workspace = Guarded_Realloc(*workspace, sizeof(double)*( polyfit_size_workspace(n,degree) ), "polyfit workspace" );
  } else {
    *workspace = polyfit_alloc_workspace( n, degree );
  }
}

void polyfit_free_workspace( double *workspace )
{ if( workspace ) free( workspace );
}

// Use SVD to solve linear least squares problem
// result is statically allocated
void polyfit_reuse( double *y, int n, int degree, double *coeffs, double *workspace )
{ int ncoeffs = degree + 1;
  double *u = workspace,
         *w = u + n*ncoeffs,
         *v = w + ncoeffs;

#ifdef DEBUG_POLYFIT_REUSE
  printf("\n---Got---\n");
  printf("\nU:\n");
  mat_print( u, n, ncoeffs);
  printf("\nS:\n");
  mat_print( w, ncoeffs, 1);
  printf("\nV:\n");
  mat_print( v, ncoeffs, ncoeffs);
  printf("\nU S V':\n");
  mat_print(
      matmul_right_transpose_static( 
            matmul_right_vec_as_diag_static( u, n, ncoeffs,
                                             w, ncoeffs),
               n, ncoeffs,
            v, ncoeffs, ncoeffs),
      n, ncoeffs);
#endif
  svd_backsub( u, w, v, n, ncoeffs, y, coeffs );
}

void polyfit( double *x, double *y, int n, int degree, double *coeffs, double *workspace )
{ int ncoeffs = degree + 1;
  double *u = workspace,   
         *w = u + n*ncoeffs,
         *v = w + ncoeffs;  
  double **iu = mat_index(u,     n,ncoeffs),
         **iv = mat_index(v,ncoeffs,ncoeffs);
  
  Vandermonde_Build(x,n,ncoeffs,u);
#ifdef DEBUG_POLYFIT
  printf("\nV:\n");
  mat_print( u, n, ncoeffs);
#endif
  svd( iu, n, ncoeffs, w, iv );  
  free(iu);
  free(iv);
  svd_threshold( POLYFIT_SINGULAR_THRESHOLD, w, ncoeffs );
#ifdef DEBUG_POLYFIT
  printf("\n---Got---\n");
  printf("\nU:\n");
  mat_print( u, n, ncoeffs);
  printf("\nS:\n");
  mat_print( w, ncoeffs, 1);
  printf("\nV:\n");
  mat_print( v, ncoeffs, ncoeffs);
  printf("\nU S V':\n");
  mat_print(
      matmul_right_transpose_static( 
            matmul_right_vec_as_diag_static( u, n, ncoeffs,
                                             w, ncoeffs),
               n, ncoeffs,
            v, ncoeffs, ncoeffs),
      n, ncoeffs);
#endif

  polyfit_reuse( y, n, degree, coeffs, workspace );
}

#ifdef TEST_POLYFIT_1
#define TEST_POLYFIT_MAIN
double X[10] = { 0, 1, 2, 3, 4,
                 5, 6, 7, 8, 9 };
double Y[10] = { 0, 1, 4, 9,16,
                25,36,49,64,81 };
double P[4]  = { 0, 0, 1, 0};
int N   = 10,
    DEG = 3;
#endif
#ifdef TEST_POLYFIT_2
#define TEST_POLYFIT_MAIN
double X[10] = { 0.03291276,  0.09627405,  0.18842924,  0.21204188,  0.23496295,
                 0.58224002,  0.84938189,  0.89690195,  0.90761286,  0.9866041  };
double Y[10] = { 0.32217143,  0.32532814,  0.33199861,  0.33410435,  0.33630316,
                 0.38826886,  0.45205618,  0.46557221,  0.46870917,  0.49287179 };
double P[3]  = { 0.3209914 ,  0.03107847,  0.14507914 }; 
int N   = 10,
    DEG = 2;
#endif

#ifdef TEST_POLYFIT_MAIN
int main(int argc, char* argv[])
{ double p[10], *workspace;
  workspace = polyfit_alloc_workspace( N, DEG );
  polyfit( X, Y, N, DEG, p, workspace );
  polyfit_free_workspace(workspace);

  printf("--Expected--\n");
  mat_print( P, DEG+1, 1 );
  printf("--   Got  --\n");
  mat_print( p, DEG+1, 1 );

  { //do polyval check
    int i;
    double tol = 1e-6;
    for(i=0; i<N; i++)
      assert( fabs( Y[i] - polyval(p,DEG,X[i]) ) < tol );
  }
  return 0;
}
#endif
