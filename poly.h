#ifndef H_POLY
// TODO: It would be much nicer to structure this for 
//       stack based operations.  e.g
//
//       poly_push(p,degree);
//       poly_der(2)
//       poly_push(q,degree2);
//       poly_mul();
//       degree3 = poly_pop(r);
//

inline double polyval             ( double *p, int degree, double x);
inline int    polymul_nelem_dest  ( int na, int nb);
inline void   polymul             ( double *a, int na, double *b, int nb, double *dest );
inline void   polyadd_ip_left     ( double *a, int na, double *b, int nb );
inline void   polysub_ip_left     ( double *a, int na, double *b, int nb );
inline void   polyadd             ( double *a, int na, double *b, int nb, double *dest );
inline void   polysub             ( double *a, int na, double *b, int nb, double *dest );
inline void   polyder_ip          ( double *a, int na, int times);

void    Vandermonde_Build             ( double *x, int n, int degree, double *V );
void    Vandermonde_Inverse           ( double *x, int n, double *invV);
double  Vandermonde_Determinant       ( double *x, int n );
double  Vandermonde_Determinant_Log2  ( double *x, int n );
inline int polyfit_size_workspace     ( int n, int degree );
double *polyfit_alloc_workspace       ( int n, int degree );
void    polyfit_realloc_workspace     ( int n, int degree, double **workspace );
void    polyfit_free_workspace        ( double *workspace );
void    polyfit_reuse                 ( double *y, int n, int degree, double *coeefs, double *workspace );
void    polyfit                       ( double *x, double *y, int n, int degree, double *coeffs, double *workspace );
#endif
