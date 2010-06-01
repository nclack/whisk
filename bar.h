#ifndef H_WHISK_BAR
#define H_WHISK_BAR

#include "compat.h"
#include "image_lib.h"

/* Finds the center of a circular object present in the image.
 *
 * The image should be scaled so the circular object is bright on a dark
 * background.
 *
 * The approach used is based on analyzing the shape of contours extracted from
 * the level-set decomposition of the image.  Each level set yields a
 * discretely sampled closed contour.  For each point in a contour, a circle
 * circumscribing the triangle formed from the point, and the two neighbors
 * `gap` points away along the contour.
 *
 * A histogram of the circle centers is computed.  The peak is the estimated
 * center.  The resolution of the histogram determines the precision limit of
 * the output: currently half pixel precision is used.
 *
 * The parameter with the most impact on the result is the `gap`.  The others
 * may be used to improve speed by culling the contours considered for the more
 * expensive circle computation.  The `gap` should be large enough to resolve
 * the expected curvature, but small enough to ensure the expected circle is
 * sufficiently sampled.  As a rule of thumb:
 *
 *    gap = expected_perimeter / 8 = pi/4 * radius ~= 0.75 * radius
 * 
 * So, for an expected radius of 19, the `gap` should be 15.
 *
 */ 
SHARED_EXPORT
void Compute_Bar_Location(  Image *im,      // The bar should be bright.
                            double *x,      // Output: x position
                            double *y,      // Output: y position
                            int gap,        // Neighbor distance 
                            int minlen,     // Minimum length of considered contours (in pixels)
                            int lvl_low,    // Minimum intensity to consider
                            int lvl_high,   // Maximum intensity to consider
                            double r_low,   // Minimum circumscribed radius to consider
                            double r_high );// Maximum circumscribed radius to consider 

#endif //H_WHISK_BAR
