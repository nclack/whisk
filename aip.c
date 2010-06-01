/* Area of intersecting polygons.
 *
 * Code based on that by Norman Hardy fetched from
 * http://www.cap-lore.com/MathPhys/IP/
 *
 * Modifications and comments by Nathan Clack.
 *
 * The intersection area is computed by integrating winding numbers.  An
 * integral is computed for one polygon over its area. The integrand is the
 * winding number for a point with respect to the _other_ polygon. The winding
 * number is zero everywhere but inside the _other_ polygon, so the integral
 * yields the area of the intersection.
 *
 * Applying Green's theorem, the area integral can be converted to one around
 * the polygon contours.  The winding number has to be evaluated on the edges
 * now.  To do this one can compute the winding number for a single vertex
 * (with respect to the other polygon) and then propagate that number along the
 * contour accounting for intersections by incrementing for a positive cross
 * and decrementing for a negative cross.
 *
 * Since intersections must be computed, some effort has to be made to avoid
 * numerical degeneracies.  Here this is solved using the "Simulation of
 * simplicity" approach.  See `fit` for more details.  Briefly, points are
 * mapped on to an integral lattice which is then perturbed systematically
 * before computing primitives such as determinants to guarantee degeneracies
 * (such as a zero determinant) are impossible.
 *
 * Scales as O( N*M ) where N and M are the vertexes in each polygon
 * respectively. 
 */
#include "compat.h"
#include "aip.h"
#include <stdlib.h>

inline void bdr(real * X, real y)
{ *X = *X<y ? *X:y;
}

inline void bur(real * X, real y)
{ *X = *X>y ? *X:y;
}

void range(box *B, point * x, int c)
{ while(c--)
  { bdr(&B->min.x, x[c].x); bur(&B->max.x, x[c].x);
    bdr(&B->min.y, x[c].y); bur(&B->max.y, x[c].y);
  }
}

double fit(box *B, point * x, int cx, vertex * ix, int fudge)
  /* Fits points to an integral lattice.
   *
   * Converts floating point coords to an integer representation.  The bottom
   * three bits beyond the significance of the floating point and are used to
   * offset points to guarantee resolution of degeneracies.  This is similar to
   * the method described in:
   *
   * Edelsbrunner, H. and Mücke, E. P. Simulation of simplicity
   * ACM Trans. Graph. 9, 1 (1990), 66-104. 
   * http://doi.acm.org/10.1145/77635.77639
   */
{ const real gamut = 500000000., mid = gamut/2.;
  real rngx = B->max.x - B->min.x, sclx = gamut/rngx,
        rngy = B->max.y - B->min.y, scly = gamut/rngy;
  int c=cx;
  while(c--)
  { ix[c].ip.x = (long)((x[c].x - B->min.x)*sclx - mid)&~7|fudge|c&1;
    ix[c].ip.y = (long)((x[c].y - B->min.y)*scly - mid)&~7|fudge;
  }
  ix[0].ip.y += cx&1;
  ix[cx] = ix[0];
  
  c=cx; 
  while(c--) 
  { rng xl = { ix[c  ].ip.x, ix[c+1].ip.x },
        xh = { ix[c+1].ip.x, ix[c  ].ip.x },
        yl = { ix[c  ].ip.y, ix[c+1].ip.y },
        yh = { ix[c+1].ip.y, ix[c  ].ip.y };
    ix[c].rx = ix[c].ip.x < ix[c+1].ip.x ? xl : xh;
//      (rng){ ix[c  ].ip.x, ix[c+1].ip.x }:
//      (rng){ ix[c+1].ip.x, ix[c  ].ip.x };
    ix[c].ry = ix[c].ip.y < ix[c+1].ip.y ? yl : yh;
//      (rng){ ix[c  ].ip.y, ix[c+1].ip.y}:
//      (rng){ ix[c+1].ip.y, ix[c  ].ip.y};
    ix[c].in=0;
  }
  return sclx*scly;
}

inline hp area(ipoint a, ipoint p, ipoint q)
/* Compute the area of the triangle (apq) */
{ return (hp)p.x*q.y - (hp)p.y*q.x +
    (hp)a.x*(p.y - q.y) + (hp)a.y*(q.x - p.x);
}

inline void cntrib(hp *s, ipoint f, ipoint t, short w)
/* Integrand for the line integral.  Google `Green's theorem polygon area` for
 * functional form.
 */
{ *s += (hp)w*(t.x-f.x)*(t.y+f.y)/2;
}

inline int ovl(rng p, rng q)
/* True if intervals intersect */
{ return p.mn < q.mx && q.mn < p.mx;
}

void cross(hp *s, vertex * a, vertex * b, vertex * c, vertex * d,
    double a1, double a2, double a3, double a4)
{ /* Interpolate to intersection and add contributions from each half edge */
  real r1=a1/((real)a1+a2), r2 = a3/((real)a3+a4);
  /*
  cntrib(s, (ipoint){a->ip.x + r1*(b->ip.x-a->ip.x), 
                     a->ip.y + r1*(b->ip.y-a->ip.y)},
      b->ip, 1);
  cntrib(s, d->ip, (ipoint){
      c->ip.x + r2*(d->ip.x - c->ip.x), 
      c->ip.y + r2*(d->ip.y - c->ip.y)}, 1);
  */
  { ipoint p = {  a->ip.x + r1*(b->ip.x-a->ip.x), 
                  a->ip.y + r1*(b->ip.y-a->ip.y)};
    cntrib(s, p, b->ip, 1 );
  }
  { ipoint p = {
      c->ip.x + r2*(d->ip.x - c->ip.x), 
      c->ip.y + r2*(d->ip.y - c->ip.y)};
    cntrib(s, d->ip, p, 1 );
  }
  ++a->in; /* Track winding numbers...these show up later in `inness` */
  --c->in;
}

void inness(hp *sarea, vertex * P, int cP, vertex * Q, int cQ)
{ int s=0, c=cQ; 
  ipoint p = P[0].ip;
  int j; 
  while(c--) //Compute winding of P[0] wrt Q
    if(Q[c].rx.mn < p.x && p.x < Q[c].rx.mx) //Bounds check x-interval only 
    {  //use area to determine P[0] left of Q[c] edge 
      int sgn = 0 < area(p, Q[c].ip, Q[c+1].ip);   // 0 or 1. 1 if left of Q[c] and Q[c] moves right
      // only count cw and moving right or ccw and moving left
      s += sgn != Q[c].ip.x < Q[c+1].ip.x ? 0 : (sgn?-1:1); //add winding
    }
  for(j=0; j<cP; ++j)
  { if(s) 
      cntrib(sarea, P[j].ip, P[j+1].ip, s);
    s += P[j].in;
  }
}

real inter(point * a, int na, point * b, int nb)
{ //vertex ipa[na+1], ipb[nb+1];
  vertex *ipa,*ipb;
  box B = {{bigReal, bigReal}, {-bigReal, -bigReal}};
  double ascale;

  if(na < 3 || nb < 3) return 0;
  ipa = (vertex*) malloc( sizeof(vertex) * (na+1) );
  ipb = (vertex*) malloc( sizeof(vertex) * (nb+1) );

  range(&B, a, na); 
  range(&B, b, nb);

  ascale = fit(&B, a, na, ipa, 0); 
  ascale = fit(&B, b, nb, ipb, 2);

  { hp s = 0; 
    int j, k;

    /*
     * Look for crossings, add contributions from crossings and track winding
     * */
    for(j=0; j<na; ++j) 
      for(k=0; k<nb; ++k)
        if(ovl(ipa[j].rx, ipb[k].rx) && ovl(ipa[j].ry, ipb[k].ry)) // if edges have overlapping bounding boxes...
        { 
          hp a1 = -area(ipa[j  ].ip, ipb[k].ip, ipb[k+1].ip),
             a2 =  area(ipa[j+1].ip, ipb[k].ip, ipb[k+1].ip);
          { int o = a1<0; 
            if(o == a2<0) //if there's a crossing...
            {
              hp a3 = area(ipb[k].ip, ipa[j].ip, ipa[j+1].ip),
                 a4 = -area(ipb[k+1].ip, ipa[j].ip, ipa[j+1].ip);
              if(a3<0 == a4<0)  //if still consistent with a crossing...
              { if(o) cross(&s, &ipa[j], &ipa[j+1], &ipb[k], &ipb[k+1], a1, a2, a3, a4);
                else  cross(&s, &ipb[k], &ipb[k+1], &ipa[j], &ipa[j+1], a3, a4, a1, a2);
              }
            }
          }
        }
    /* Add contributions from non-crossing edges */
    inness(&s, ipa, na, ipb, nb); 
    inness(&s, ipb, nb, ipa, na);
    free(ipa);
    free(ipb);
    return s/ascale;
  }
}

#ifdef TEST_AIP
int main(){
  point
#ifdef TEST_AIP_1
       a[] = {{2,3}, {2,3}, {2,3}, {2,4}, {3,3}, {2,3}, {2,3}},
       b[] = {{1,1}, {1,4}, {4,4}, {4,1}, {1,1}}; // 1/2, 1/2
    // The redundant vertexes above are to provoke errors as good test cases should.
    // It is not necessary to duplicate the first vertex at the end.
#endif
#ifdef TEST_AIP_2
    a[] = {{1,7}, {4,7}, {4, 6}, {2,6}, {2, 3}, {4,3}, {4,2}, {1,2}},
    b[] = {{3,1}, {5,1}, {5,4}, {3,4}, {3,5}, {6,5}, {6,0}, {3,0}}; // 0, 9
#endif
#ifdef TEST_AIP_3
    a[] = {{1,1}, {1,2}, {2,1}, {2,2}},
    b[] = {{0,0}, {0,4}, {4,4}, {4,0}}; // 0, 1/2
#endif
#ifdef TEST_AIP_4
    a[] = {{0,0}, {3,0}, {3,2}, {1,2}, {1,1}, {2,1}, {2,3}, {0,3}},
    b[] = {{0,0}, {0,4}, {4,4}, {4,0}}; // -9, 11
#endif
#ifdef TEST_AIP_5
     a[] = {{0,0}, {1,0}, {0,1}},
     b[] = {{0,0}, {0,1}, {1,1}, {1,0}}; // -1/2, 1/2
#endif
#ifdef TEST_AIP_6
     a[] = {{1,3}, {2,3}, {2,0}, {1,0}},
     b[] = {{0,1}, {3,1}, {3,2}, {0,2}}; // -1, 3
#endif
#ifdef TEST_AIP_7
     a[] ={{0,0}, {0,2}, {2,2}, {2,0}},
     b[] = {{1, 1}, {3, 1}, {3, 3}, {1, 3}}; // -1, 4
#endif
#ifdef TEST_AIP_8
     a[] = {{0,0}, {0,4}, {4,4}, {4,0}},
     b[] = {{1,1}, {1,2}, {2,2}, {2,1}}; // 1, 16
#endif
  {const int ac = sizeof(a)/sizeof(a[0]), bc = sizeof(b)/sizeof(b[0]);
    printf("%e, %e\n", inter(a, ac, b, bc), inter(a, ac, a, ac)); 
    return 0;
  }
}
#endif
