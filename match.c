/*
 *  match.c
 *
 *  Adapted from Markus Buehren's implimentation of the Munkres (a.k.a. Hungarian) algorithm.
 *
 *  Adapted by Nathan Clack on 3/5/08.
 *  Copyright 2008 HHMI. All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <float.h>
#include "common.h"
#include "match.h"

#undef COLUMN_ORDER
#define CHECK_FOR_INF
#undef ONE_INDEXING
#undef MATLAB

#ifndef INFINITY
  #define INFINITY FLT_MAX
#endif

#ifndef isfinite
#define isfinite( x )  (fabs(x)<FLT_MAX)
#endif

#ifndef isinf
#define isinf(x) (!isfinite(x))
#endif

SHARED_EXPORT void assignmentoptimal(double *assignment, double *cost, double *distMatrix, int nOfRows, int nOfColumns);
void buildassignmentvector(double *assignment, char *starMatrix, int nOfRows, int nOfColumns);
void computeassignmentcost(double *assignment, double *cost, double *distMatrix, int nOfRows);
void step2a(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim);
void step2b(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim);
void step3 (double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim);
void step4 (double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim, int row, int col);
void step5 (double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim);

SHARED_EXPORT Assignment match( double* distMatrixIn, int nOfRows, int nOfColumns )
{	Assignment result;

  result.n = nOfRows;
	
  result.assignment = (double*) malloc( sizeof(double)*nOfRows );
  
  assignmentoptimal( result.assignment, &result.cost, distMatrixIn, nOfRows, nOfColumns );

  return result;
}
 
SHARED_EXPORT void assignmentoptimal(double *assignment, double *cost, double *distMatrixIn, int nOfRows, int nOfColumns)
{
	double *distMatrix, *distMatrixTemp, *distMatrixEnd, *columnEnd, value, minValue;
	char *coveredColumns, *coveredRows, *starMatrix, *newStarMatrix, *primeMatrix;
	int nOfElements, minDim, row, col;
#ifdef CHECK_FOR_INF
	char infiniteValueFound;
	double maxFiniteValue, infValue;
#endif

	/* initialization */
	*cost = 0;
	for(row=0; row<nOfRows; row++)
#ifdef ONE_INDEXING
		assignment[row] =  0.0;
#else
		assignment[row] = -1.0;
#endif

	/* generate working copy of distance Matrix */
	/* check if all matrix elements are positive */
	nOfElements   = nOfRows * nOfColumns;
	distMatrix    = (double *)malloc(nOfElements * sizeof(double));
	distMatrixEnd = distMatrix + nOfElements;
	for(row=0; row<nOfElements; row++)
	{
		value = distMatrixIn[row];
		if(isfinite(value) && (value < 0))
			fprintf(stderr, "All matrix elements have to be non-negative.\n");
#ifdef COLUMN_ORDER
    /* translate an input column-order matrix into row-order form */
    distMatrix[ ((int) row/nOfColumns) + nOfRows * (row%nOfColumns) ] = value;
#else
		distMatrix[row] = value;
#endif
	}

#ifdef CHECK_FOR_INF
	/* check for infinite values */
	maxFiniteValue     = -1;
	infiniteValueFound = 0;

	distMatrixTemp = distMatrix;
	while(distMatrixTemp < distMatrixEnd)
	{
		value = *distMatrixTemp++;
		if(isfinite(value))
		{
			if(value > maxFiniteValue)
				maxFiniteValue = value;
		}
		else
			infiniteValueFound = 1;
	}
	if(infiniteValueFound)
	{
		if(maxFiniteValue == -1) /* all elements are infinite */
			return;

		/* set all infinite elements to big finite value */
		if(maxFiniteValue > 0)
			infValue = 10 * maxFiniteValue * nOfElements;
		else
			infValue = 10;
		distMatrixTemp = distMatrix;
		while(distMatrixTemp < distMatrixEnd)
			if(isinf(*distMatrixTemp++))
				*(distMatrixTemp-1) = infValue;
	}
#endif

	/* memory allocation */
	coveredColumns = (char *)malloc(nOfColumns*  sizeof(char));
        memset( coveredColumns, 0, nOfColumns     *  sizeof(char));
	coveredRows    = (char *)malloc(nOfRows   *  sizeof(char));
        memset( coveredRows   , 0, nOfRows        *  sizeof(char));
	starMatrix     = (char *)malloc(nOfElements* sizeof(char));
        memset( starMatrix    , 0, nOfElements    *  sizeof(char));
	primeMatrix    = (char *)malloc(nOfElements* sizeof(char));
        memset( primeMatrix   , 0, nOfElements    *  sizeof(char));
        newStarMatrix  = (char *)malloc(nOfElements* sizeof(char)); /* used in step4 */
        memset( newStarMatrix , 0, nOfElements    *  sizeof(char));

	/* preliminary steps */
	if(nOfRows <= nOfColumns)
	{
		minDim = nOfRows;

		for(row=0; row<nOfRows; row++)
		{
			/* find the smallest element in the row */
			distMatrixTemp = distMatrix + row;
			minValue = *distMatrixTemp;
			distMatrixTemp += nOfRows;
			while(distMatrixTemp < distMatrixEnd)
			{
				value = *distMatrixTemp;
				if(value < minValue)
					minValue = value;
				distMatrixTemp += nOfRows;
			}

			/* subtract the smallest element from each element of the row */
			distMatrixTemp = distMatrix + row;
			while(distMatrixTemp < distMatrixEnd)
			{
				*distMatrixTemp -= minValue;
				distMatrixTemp += nOfRows;
			}
		}

		/* Steps 1 and 2a */
		for(row=0; row<nOfRows; row++)
			for(col=0; col<nOfColumns; col++)
				if(distMatrix[row + nOfRows*col] == 0)
					if(!coveredColumns[col])
					{
						starMatrix[row + nOfRows*col] = 1;
						coveredColumns[col]           = 1;
						break;
					}
	}
	else /* if(nOfRows > nOfColumns) */
	{
		minDim = nOfColumns;

		for(col=0; col<nOfColumns; col++)
		{
			/* find the smallest element in the column */
			distMatrixTemp = distMatrix     + nOfRows*col;
			columnEnd      = distMatrixTemp + nOfRows;

			minValue = *distMatrixTemp++;
			while(distMatrixTemp < columnEnd)
			{
				value = *distMatrixTemp++;
				if(value < minValue)
					minValue = value;
			}

			/* subtract the smallest element from each element of the column */
			distMatrixTemp = distMatrix + nOfRows*col;
			while(distMatrixTemp < columnEnd)
				*distMatrixTemp++ -= minValue;
		}

		/* Steps 1 and 2a */
		for(col=0; col<nOfColumns; col++)
			for(row=0; row<nOfRows; row++)
				if(distMatrix[row + nOfRows*col] == 0)
					if(!coveredRows[row])
					{
						starMatrix[row + nOfRows*col] = 1;
						coveredColumns[col]           = 1;
						coveredRows[row]              = 1;
						break;
					}
		for(row=0; row<nOfRows; row++)
			coveredRows[row] = 0;

	}

	/* move to step 2b */
	step2b(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);

	/* compute cost and remove invalid assignments */
	computeassignmentcost(assignment, cost, distMatrixIn, nOfRows);

	/* free allocated memory */
	free(distMatrix);
	free(coveredColumns);
	free(coveredRows);
	free(starMatrix);
	free(primeMatrix);
	free(newStarMatrix);

	return;
}

/********************************************************/
void buildassignmentvector(double *assignment, char *starMatrix, int nOfRows, int nOfColumns)
{
	int row, col;

	for(row=0; row<nOfRows; row++)
		for(col=0; col<nOfColumns; col++)
			if(starMatrix[row + nOfRows*col])
			{
#ifdef ONE_INDEXING
				assignment[row] = col + 1; /* MATLAB-Indexing */
#else
				assignment[row] = col;
#endif
				break;
			}
}

/********************************************************/
void computeassignmentcost(double *assignment, double *cost, double *distMatrix, int nOfRows)
{
	int row, col;
#ifdef CHECK_FOR_INF
	double value;
#endif

	for(row=0; row<nOfRows; row++)
	{
#ifdef ONE_INDEXING
		col = assignment[row]-1; /* MATLAB-Indexing */
#else
		col = assignment[row];
#endif

		if(col >= 0)
		{
#ifdef CHECK_FOR_INF
			value = distMatrix[row + nOfRows*col];
			if(isfinite(value))
				*cost += value;
			else
#ifdef ONE_INDEXING
				assignment[row] =  0.0;
#else
				assignment[row] = -1.0;
#endif

#else
			*cost += distMatrix[row + nOfRows*col];
#endif
		}
	}
}

/********************************************************/
void step2a(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim)
{
	char *starMatrixTemp, *columnEnd;
	int col;

	/* cover every column containing a starred zero */
	for(col=0; col<nOfColumns; col++)
	{
		starMatrixTemp = starMatrix     + nOfRows*col;
		columnEnd      = starMatrixTemp + nOfRows;
		while(starMatrixTemp < columnEnd){
			if(*starMatrixTemp++)
			{
				coveredColumns[col] = 1;
				break;
			}
		}
	}

	/* move to step 3 */
	step2b(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}

/********************************************************/
void step2b(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim)
{
	int col, nOfCoveredColumns;

	/* count covered columns */
	nOfCoveredColumns = 0;
	for(col=0; col<nOfColumns; col++)
		if(coveredColumns[col])
			nOfCoveredColumns++;

	if(nOfCoveredColumns == minDim)
	{
		/* algorithm finished */
		buildassignmentvector(assignment, starMatrix, nOfRows, nOfColumns);
	}
	else
	{
		/* move to step 3 */
		step3(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
	}

}

/********************************************************/
void step3(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim)
{
	char zerosFound;
	int row, col, starCol;

	zerosFound = 1;
	while(zerosFound)
	{
		zerosFound = 0;
		for(col=0; col<nOfColumns; col++)
			if(!coveredColumns[col])
				for(row=0; row<nOfRows; row++)
					if((!coveredRows[row]) && (distMatrix[row + nOfRows*col] == 0))
					{
						/* prime zero */
						primeMatrix[row + nOfRows*col] = 1;

						/* find starred zero in current row */
						for(starCol=0; starCol<nOfColumns; starCol++)
							if(starMatrix[row + nOfRows*starCol])
								break;

						if(starCol == nOfColumns) /* no starred zero found */
						{
							/* move to step 4 */
							step4(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim, row, col);
							return;
						}
						else
						{
							coveredRows[row]        = 1;
							coveredColumns[starCol] = 0;
							zerosFound              = 1;
							break;
						}
					}
	}

	/* move to step 5 */
	step5(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}

/********************************************************/
void step4(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim, int row, int col)
{
	int n, starRow, starCol, primeRow, primeCol;
	int nOfElements = nOfRows*nOfColumns;

	/* generate temporary copy of starMatrix */
	for(n=0; n<nOfElements; n++)
		newStarMatrix[n] = starMatrix[n];

	/* star current zero */
	newStarMatrix[row + nOfRows*col] = 1;

	/* find starred zero in current column */
	starCol = col;
	for(starRow=0; starRow<nOfRows; starRow++)
		if(starMatrix[starRow + nOfRows*starCol])
			break;

	while(starRow<nOfRows)
	{
		/* unstar the starred zero */
		newStarMatrix[starRow + nOfRows*starCol] = 0;

		/* find primed zero in current row */
		primeRow = starRow;
		for(primeCol=0; primeCol<nOfColumns; primeCol++)
			if(primeMatrix[primeRow + nOfRows*primeCol])
				break;

		/* star the primed zero */
		newStarMatrix[primeRow + nOfRows*primeCol] = 1;

		/* find starred zero in current column */
		starCol = primeCol;
		for(starRow=0; starRow<nOfRows; starRow++)
			if(starMatrix[starRow + nOfRows*starCol])
				break;
	}

	/* use temporary copy as new starMatrix */
	/* delete all primes, uncover all rows */
	for(n=0; n<nOfElements; n++)
	{
		primeMatrix[n] = 0;
		starMatrix[n]  = newStarMatrix[n];
	}
	for(n=0; n<nOfRows; n++)
		coveredRows[n] = 0;

	/* move to step 2a */
	step2a(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}

/********************************************************/
void step5(double *assignment, double *distMatrix, char *starMatrix, char *newStarMatrix, char *primeMatrix, char *coveredColumns, char *coveredRows, int nOfRows, int nOfColumns, int minDim)
{
	double h, value;
	int row, col;

	/* find smallest uncovered element h */
	h = INFINITY;
	for(row=0; row<nOfRows; row++)
		if(!coveredRows[row])
			for(col=0; col<nOfColumns; col++)
				if(!coveredColumns[col])
				{
					value = distMatrix[row + nOfRows*col];
					if(value < h)
						h = value;
				}

	/* add h to each covered row */
	for(row=0; row<nOfRows; row++)
		if(coveredRows[row])
			for(col=0; col<nOfColumns; col++)
				distMatrix[row + nOfRows*col] += h;

	/* subtract h from each uncovered column */
	for(col=0; col<nOfColumns; col++)
		if(!coveredColumns[col])
			for(row=0; row<nOfRows; row++)
				distMatrix[row + nOfRows*col] -= h;

	/* move to step 3 */
	step3(assignment, distMatrix, starMatrix, newStarMatrix, primeMatrix, coveredColumns, coveredRows, nOfRows, nOfColumns, minDim);
}


 /****************************************
 **
 ** TEST CASE
 **
 ****************************************/
#ifdef TEST_MATCH
#define M  10
#define N  1
int B[5*5] =    { 7, 2, 1, 9, 4,
                  9, 6, 9, 5, 5,
                  3, 8, 3, 1, 8,
                  7, 9, 4, 2, 2,
                  8, 4, 7, 4, 8 };

void pmat( double* array, int m, int n )
{ int i,j;
  for( i=0; i<m; ++i )
  { for( j=0; j<n; ++j )
    printf("%10.3g, ", array[i+j*m]);
    printf("\n");
  }
}

void pimat( int* array, int m, int n )
{ int i,j;
  for( i=0; i<m; ++i )
  { for( j=0; j<n; ++j )
    printf("%7d, ", array[i+j*m]);
    printf("\n");
  }
}


void pcmat( char* array, int m, int n )
{ int i,j;
  for( i=0; i<m; ++i )
  { for( j=0; j<n; ++j )
    printf("%7d, ", (int)array[i+j*m]);
    printf("\n");
  }
}


void f()
{ int i,j;
  double A[M*N];
  Assignment result;

  for( i=0; i<M*N; i++) A[i] = fabs( ((int)rand())/1024./1024./1024. );

  //printf("*************\n");
  //pmat( A, M,N );

  result = match( A, M, N );

  printf("----\n");
  pmat( A, M,N );

  for( i=0; i<result.n; i++ )
    printf( "%d --> %g\n", i, result.assignment[i] );

  printf("Cost: %g\n", result.cost);

}

int main(int argc, char* argv[] )
{ int i;
  srand( time(NULL) );
  for(i=0;i<1; i++)
    f();
  return 0;
}
#endif
