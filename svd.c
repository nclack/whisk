/* 
 * svdcomp - SVD decomposition routine. 
 * Takes an mxn matrix a and decomposes it into udv, where u,v are
 * left and right orthogonal transformation matrices, and d is a 
 * diagonal matrix of singular values.
 *
 * This routine is adapted from svdecomp.c in XLISP-STAT 2.1 which is 
 * code from Numerical Recipes adapted by Luke Tierney and David Betz.
 *
 * Input to dsvd is as follows:
 *   a = mxn matrix to be decomposed, gets overwritten with u
 *   m = row dimension of a
 *   n = column dimension of a
 *   w = returns the vector of singular values of a
 *   v = returns the right orthogonal transformation matrix
*/
#include "compat.h"
#include <math.h>
#include <assert.h>
#include <string.h>
#include "utilities.h"
#include "common.h"
#include "error.h"
#include "mat.h"

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

inline void svd_threshold( double thresh, double *w, int n )
{ double *e = w+n;
  while(e-- > w)
    if(fabs(*e) < thresh)
      *e = 0.0;  
}

//
// Solves A x = b using SVD of A ( U W V' = A, W = diag(w) )
//     That is: x = V U' b / W
//
inline void svd_backsub( double *u, double *w, double *v, int nrows, int ncols, double *b, double *x )
{ double *utb;
  // U'b
  utb = matmul_left_transpose_static( u, nrows, ncols,
                                      b, nrows, 1 );
  // (U'b)/W
  { double *e = utb + ncols,
          *we =   w + ncols;
    while(e >= utb)
      *e-- /= *we--;
  }
  // V ( U'b/W )
  matmul (v  , ncols, ncols, 
         utb, ncols, 1,
         x  );
  return;
}

// return 0 or 1 based on whether iterations converged (1) or didn't (0).
int svd(double **a, int nrows, int ncols, double *w, double **v)
{
    int flag, i, its, j, jj, k, l, nm;
    double c, f, h, s, x, y, z;
    double anorm = 0.0, g = 0.0, scale = 0.0;
    static double *rv1 = NULL;
    static size_t  rv1_size = 0;
    
    rv1 = request_storage( rv1, &rv1_size, sizeof(double), ncols, "svd" );

    /* Householder reduction to bidiagonal form */
    for (i = 0; i < ncols; i++) 
    {
        /* left-hand reduction */
        l = i + 1;
        rv1[i] = scale * g;
        g = s = scale = 0.0;
        if (i < nrows) 
        {
            for (k = i; k < nrows; k++) 
                scale += fabs(a[k][i]);
            if (scale) 
            {
                for (k = i; k < nrows; k++) 
                {
                    a[k][i] /= scale;
                    s += (a[k][i] * a[k][i]);
                }
                f = a[i][i];
                g = -SIGN(sqrt(s), f);
                h = f * g - s;
                a[i][i] = (f - g);
                for (j = l; j <  ncols ; j++) 
                {
                    for (s = 0.0, k = i; k < nrows; k++) 
                        s += (a[k][i] * a[k][j]);
                    f = s / h;
                    for (k = i; k < nrows; k++) 
                        a[k][j] += (f * a[k][i]);
                }
                for (k = i; k < nrows; k++) 
                    a[k][i] = (a[k][i]*scale);
            }
        }
        w[i] = (scale * g);
    
        /* right-hand reduction */
        g = s = scale = 0.0;
        if (i < nrows && i !=  (ncols - 1) )
        {
            for (k = l; k <  ncols ; k++) 
                scale += fabs(a[i][k]);
            if (scale) 
            {
                for (k = l; k <  ncols ; k++) 
                {
                    a[i][k] /= scale;
                    s += (a[i][k] * a[i][k]);
                }
                f = a[i][l];
                g = -SIGN(sqrt(s), f);
                h = f * g - s;
                a[i][l] = (f - g);
                for (k = l; k <  ncols ; k++) 
                    rv1[k] = a[i][k] / h;
                for (j = l; j < nrows; j++) 
                {
                    for (s = 0.0, k = l; k <  ncols ; k++) 
                        s += (a[j][k] * a[i][k]);
                    for (k = l; k <  ncols ; k++) 
                        a[j][k] += (s * rv1[k]);
                }
                for (k = l; k <  ncols ; k++) 
                    a[i][k] *= scale;
            }
        }
        anorm = MAX( anorm, ( fabs(w[i]) + fabs(rv1[i]) ) );
    }
  
    /* accumulate the right-hand transformation */
    for (i =  ncols  - 1; i >= 0; i--) 
    {
        if (i <  ncols  - 1) 
        {
            if (g) 
            {
                for (j = l; j <  ncols ; j++)
                    v[j][i] = ((a[i][j] / a[i][l]) / g);
                    /* double division to avoid underflow */
                for (j = l; j <  ncols ; j++) 
                {
                    for (s = 0.0, k = l; k <  ncols ; k++) 
                        s += (a[i][k] * v[k][j]);
                    for (k = l; k <  ncols ; k++) 
                        v[k][j] += (s * v[k][i]);
                }
            }
            for (j = l; j <  ncols ; j++) 
                v[i][j] = v[j][i] = 0.0;
        }
        v[i][i] = 1.0;
        g = rv1[i];
        l = i;
    }
  
    /* accumulate the left-hand transformation */
    for (i =  MIN(nrows,ncols) - 1; i >= 0; i--) 
    {
        l = i + 1;
        g = w[i];
        for (j = l; j <  ncols ; j++) 
            a[i][j] = 0.0;
        if (g) 
        {
            g = 1.0 / g;
            for (j = l; j <  ncols ; j++) 
            {
                for (s = 0.0, k = l; k < nrows; k++) 
                    s += (a[k][i] * a[k][j]);
                f = (s / a[i][i]) * g;
                for (k = i; k < nrows; k++) 
                    a[k][j] += (f * a[k][i]);
            }
            for (j = i; j < nrows; j++) 
                a[j][i] *= g;
        }
        else 
        {
            for (j = i; j < nrows; j++) 
                a[j][i] = 0.0;
        }
        ++a[i][i];
    }

    /* diagonalize the bidiagonal form */
    for (k =  ncols  - 1; k >= 0; k--) 
    {                             /* loop over singular values */
        for (its = 0; its < 30; its++) 
        {                         /* loop over allowed iterations */
            flag = 1;
            for (l = k; l >= 0; l--) 
            {                     /* test for splitting */
                nm = l - 1;
                if (fabs(rv1[l]) + anorm == anorm) 
                {
                    flag = 0;
                    break;
                }
                if (fabs(w[nm]) + anorm == anorm) 
                    break;
            }
            if (flag) 
            {
                c = 0.0;
                s = 1.0;
                for (i = l; i <= k; i++) 
                {
                    f = s * rv1[i];
                    if (fabs(f) + anorm == anorm) break;
                    g = w[i];
                    h = hypot(f, g);
                    w[i] = h; 
                    h = 1.0 / h;
                    c = g * h;
                    s = (- f * h);
                    for (j = 0; j < nrows; j++) 
                    {
                        y = a[j][nm];
                        z = a[j][i];
                        a[j][nm] = (y * c + z * s);
                        a[j][i] = (z * c - y * s);
                    }
                }
            }
            z = w[k];
            if (l == k) 
            {                  /* convergence */
                if (z < 0.0) 
                {              /* make singular value nonnegative */
                    w[k] = (-z);
                    for (j = 0; j <  ncols ; j++) 
                        v[j][k] = -v[j][k];
                }
                break;
            }
#ifdef DEBUG_SVD
            debug("Iterations: %d\n",its);
#endif
            if (its >= 30) {
                warning("SVD: No convergence after 30,000! iterations \n");
                return(0);
            }
    
            /* shift from bottom 2 x 2 minor */
            x = w[l];
            nm = k - 1;
            y = w[nm];
            g = rv1[nm];
            h = rv1[k];
            f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
            g = hypot(f, 1.0);
            f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g, f))) - h)) / x;
          
            /* next QR transformation */
            c = s = 1.0;
            for (j = l; j <= nm; j++) 
            {
                i = j + 1;
                g = rv1[i];
                y = w[i];
                h = s * g;
                g = c * g;
                z = hypot(f, h);
                rv1[j] = z;
                c = f / z;
                s = h / z;
                f = x * c + g * s;
                g = g * c - x * s;
                h = y * s;
                y *= c;
                for (jj = 0; jj <  ncols ; jj++) 
                {
                    x = v[jj][j];
                    z = v[jj][i];
                    v[jj][j] = (x * c + z * s);
                    v[jj][i] = (z * c - x * s);
                }
                z = hypot(f, h);
                w[j] = z;
                if (z) 
                {
                    z = 1.0 / z;
                    c = f * z;
                    s = h * z;
                }
                f = (c * g) + (s * y);
                x = (c * y) - (s * g);
                for (jj = 0; jj < nrows; jj++) 
                {
                    y = a[jj][j];
                    z = a[jj][i];
                    a[jj][j] = (y * c + z * s);
                    a[jj][i] = (z * c - y * s);
                }
            }
            rv1[l] = 0.0;
            rv1[k] = f;
            w[k] = x;
        }
    }
    return(1);
}

#ifdef TEST_SVD_1
#define TEST_SVD_MAIN
double A[4] = { 4, 4 , 
               -3, 3};
double V[4] = {-.70710678118654752440,-.70710678118654752440,
               -.70710678118654752440, .70710678118654752440};
double S[2] = {5.65685424949238019520 ,
               4.24264068711928514640 };
double U[4] = {-1.0, 0.0,
               0.0, 1.0};
int NROWS = 2,
    NCOLS = 2;
#endif
#ifdef TEST_SVD_2
#define TEST_SVD_MAIN
double A[4] = { 4,  3 , 
               -8, -6};
double V[4] = {-0.8, 0.6,
                0.6, 0.8};
double S[2] = { 11.18033988749894848204,
                0.0 };
double U[4] = {-0.44721359549995793928,  0.89442719099991587856,
                0.89442719099991587856,  0.44721359549995793928 };
int NROWS = 2,
    NCOLS = 2;
#endif
#ifdef TEST_SVD_3
#define TEST_SVD_MAIN
double A[6] = { 4,  3, 
               -8, -6,
                1,  2};
double U[9] = { -4.40118458e-01,   7.93457174e-02,
                 8.80236916e-01,  -1.58691435e-01,
                -1.77422418e-01,  -9.84134790e-01};
double S[2] = {  11.35919198,   0.98425486 };
double V[4] = {  -0.79053084,  0.61242223 ,
                 -0.61242223, -0.79053084 };
int NROWS = 3,
    NCOLS = 2;
#endif

#ifdef TEST_SVD_MAIN 

int main(int argc, char* argv[])
{ double v[4], s[2];
  double **ia = mat_index(A,NROWS,NCOLS),
         **iv = mat_index(v,NROWS,NCOLS);

  printf("Input\n");
  mat_print( A, NROWS, NCOLS);

  assert( svd(ia, NROWS, NCOLS, s, iv) );
  free(ia);
  free(iv);

  printf("\n---Got---\n");
  printf("\nU:\n");
  mat_print( A, NROWS, NCOLS);
  printf("\nS:\n");
  mat_print( s, NCOLS, 1);
  printf("\nV:\n");
  mat_print( v, NCOLS, NCOLS);
  printf("\nU S V':\n");
  mat_print(
      matmul_right_transpose_static( 
            matmul_right_vec_as_diag_static( A, NROWS, NCOLS,
                                             s, NCOLS),
               NROWS, NCOLS,
            v, NCOLS, NCOLS),
      NROWS, NCOLS);

  printf("\n---Expected---\n");
  printf("\nU:\n");
  mat_print( U, NROWS, NCOLS);
  printf("\nS:\n");
  mat_print( S, NCOLS, 1);
  printf("\nV:\n");
  mat_print( V, NCOLS, NCOLS);

  printf("\nU S V':\n");
  mat_print(
      matmul_right_transpose_static( 
            matmul_right_vec_as_diag_static( U, NROWS, NCOLS,
                                             S, NCOLS),
               NROWS, NCOLS,
            V, NCOLS, NCOLS),
      NROWS, NCOLS);

  return 0;
}
#endif
