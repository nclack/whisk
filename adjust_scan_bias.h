#ifndef  _H_ADJUST_SCAN_BIAS
#define _H_ADJUST_SCAN_BIAS

#include "image_lib.h"

double  incremental_estimate_scan_bias_h ( Image *image, double thresh, double *stat);
void    image_adjust_scan_bias_h         ( Image *image, double gain );
double  incremental_estimate_scan_bias_v ( Image *image, double thresh, double *stat);
void    image_adjust_scan_bias_v         ( Image *image, double gain );

double  estimate_scan_bias_h(Stack *movie, double mean_intensity, double *stat);
void    adjust_scan_bias_h( Stack *movie, double gain );
double  estimate_scan_bias_v(Stack *movie, double mean_intensity, double *stat);
void    adjust_scan_bias_v( Stack *movie, double gain );
void    adjust_scan_bias( Stack *movie );

#endif//_H_ADJUST_SCAN_BIAS
