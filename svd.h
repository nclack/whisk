#ifndef H_SVD

inline void svd_threshold ( double thresh, double *w, int n );

inline void svd_backsub   ( double *u, double *w, double *v, 
                            int nrows, int ncols, 
                            double *b, double *x );

        int svd           ( double **a, int nrows, int ncols, 
                            double *w, double **v);

#endif //H_SVD
