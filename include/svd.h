/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
#ifndef H_SVD

 void svd_threshold ( double thresh, double *w, int n );

 void svd_backsub   ( double *u, double *w, double *v, 
                            int nrows, int ncols, 
                            double *b, double *x );

        int svd           ( double **a, int nrows, int ncols, 
                            double *w, double **v);

#endif //H_SVD
