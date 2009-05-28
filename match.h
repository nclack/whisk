/*
 *  match.h
 *  
 *  Adapted from Markus Buehren's implimentation of the Munkres (a.k.a. Hungarian)
 *  algorithm.
 *
 *  Adapted by Nathan Clack on 3/5/08.
 *  Copyright 2008 HHMI. All rights reserved.
 *
 */

#ifndef H_MATCH
#define H_MATCH

typedef struct  
{ double  *assignment; 
  double  cost;
  int     n;
} Assignment;

Assignment match( double* distMatrixIn, int nOfRows, int nOfColumns );
void assignmentoptimal(double *assignment, double *cost, double *distMatrixIn, int nOfRows, int nOfColumns);

/* Matrix printing functions (for debuging mostly)*/
void pmat( double* array, int m, int n);
void pimat( int* array, int m, int n);
void pxmat(char* array, int m, int n);

#endif
