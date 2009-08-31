#ifndef H_MAT
void     mat_print                ( double *M, int nrows, int ncols );
double **mat_index                ( double *M, int nrows, int ncols );
void     matmul                   ( double *a, int nar, int nac, 
                                    double *b, int nbr, int nbc, 
                                    double *dest );
void     matmul_right_transpose   ( double *a, int nar, int nac, 
                                    double *b, int nbr, int nbc, 
                                    double *dest );
void     matmul_left_transpose    ( double *a, int nar, int nac, 
                                    double *b, int nbr, int nbc,
                                    double *dest );
void     matmul_left_vec_as_diag  ( double *vec, int n_vec, 
                                    double *mat, int nrows, int ncols, 
                                    double *dest );
void     matmul_right_vec_as_diag ( double *mat, int nrows, int ncols, 
                                    double *vec, int n_vec, 
                                    double *dest );
double*  matmul_static            ( double *a, int nar, int nac, 
                                    double *b, int nbr, int nbc);
double*  matmul_right_transpose_static   ( double *a, int nar, int nac, 
                                           double *b, int nbr, int nbc);
double*  matmul_left_transpose_static    ( double *a, int nar, int nac, 
                                           double *b, int nbr, int nbc);
double*  matmul_left_vec_as_diag_static  ( double *vec, int n_vec, 
                                           double *mat, int nrows, int ncols);
double*  matmul_right_vec_as_diag_static ( double *mat, int nrows, int ncols, 
                                           double *vec, int n_vec);
#endif //H_MAT
