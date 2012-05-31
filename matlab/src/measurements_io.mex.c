/* Author: Nathan Clack
 *   Date: June 14, 2010
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html). 
 */

//#define DEBUG

#ifdef LOAD_MEASUREMENTS
/**
 * mexFunction for Matlab function LoadMeasurements.
 *
 * Matlab usage:
 *
 *  Build: (also see buildmex.m)
 *
 *    mex -DLOAD_MEASUREMENTS -o LoadMeasurements measurements_io.mex -lwhisk -L./lib -I./include 
 *
 *   On windows it may be necessary to add 
 * 
 *      LINKFLAGSPOST=/NODEFAULTLIB:LIBCMT
 *
 *   to the mex command line or to your mexopts.bat. 
 *
 *  Calling from Matlab console:
 *
 *    table = LoadMeasurements('path/to/file.measurements'); 
 *
 *  Returned `table` is a `struct array` with the following fields:
 *    
 *    fid                         - Video frame where the segment was found
 *    wid                         - id for the segment on that frame
 *    label                       - identity.  For tracking use -1 for `not a whisker` and 
 *                                  0,1,2.. for whiskers.                                  
 *    face_x                      - roughly, center of whisker pad, y coordinate           
 *    face_y                      - roughly, center of whisker pad, y coordinate           
 *    length                      - path length in pixels                                  
 *    score                       - median score from tracing                              
 *    angle                       - mean angle at follicle
 *    curvature                   - mean curvature (1/mm)
 *    follicle_x                  - follicle position: x (px)
 *    follicle_y                  - follicle position: y (px)
 *    tip_x                       - tip position: x (px)
 *    tip_y                       - tip position: y (px)
 *
 */
#include <matrix.h>
#include <mex.h>
#include "measurements_io.h"

#define mxerror(msg) mexErrMsgTxt((msg))
#define mxassert(expr,msg) if(!(expr)) mxerror((msg))

#include <stdarg.h>
#define TRY(e) if(!(e)) goto Error;
void mxreport(char *str, va_list argList)
{ char *buf = NULL;
  size_t  n = 0;
  TRY(buf=calloc(n=1024,1));
  while(  ((unsigned)vsnprintf(buf,n,str,argList)) >= n ) // have to convert to unsigned for win32 version, which returns -1 on error
    TRY(buf = (char*)realloc(buf,n=(1.2*n+1024)));
  mexPrintf("[WHISK] %s",buf);
  return;
Error:
  mexPrintf("%s(%d): Something went wrong trying to print an error message.\n",__FILE__,__LINE__);
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ size_t len;
  char *filename;
  Measurements *table;
  int nrows;

  set_reporter(mxreport);

  mxassert(nrhs==1           , "One input required.");
  mxassert(nlhs<=1           , "Too many output arguments.");
  mxassert(mxIsChar(prhs[0]) , "Input must be a string.");

  len = mxGetM(prhs[0])*mxGetN(prhs[0])+1;
  mxassert(
      filename=mxArrayToString(prhs[0]),
      "Could not convert input to string.");

  mxassert(
      table=Measurements_Table_From_Filename(filename,NULL,&nrows),
      "Could not read Measurements file.");

  if(nlhs==0)
    goto Done;
  
  { const char *fields[] = {
                            "fid",
                            "wid",
                            "label",
                            "face_x",
                            "face_y",
                            "length",
                            "score",
                            "angle",
                            "curvature",
                            "follicle_x",
                            "follicle_y",
                            "tip_x",
                            "tip_y"
                           };
    plhs[0] = mxCreateStructMatrix(
              nrows
            , 1
            , sizeof(fields)/sizeof(char*)
            , fields);
    if(!plhs[0])
      goto AllocStructArrayException;

  }
  { int i;
    for(i=0;i<nrows;++i)
    { mxArray *fid,*wid,*label,*face_x,*face_y,*length,*score,*angle,*curvature,*follicle_x,*follicle_y,*tip_x,*tip_y;
      int *pfid,*pwid,*plabel,*pface_x,*pface_y;
      double *plength,*pscore,*pangle, *pcurvature,*pfollicle_x,*pfollicle_y, *ptip_x, *ptip_y;
      int n;
      
      n = table[i].n;
      fid         = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL);  
      wid         = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL);  
      label       = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL); 
      face_x      = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL); 
      face_y      = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL); 
      length      = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL); 
      score       = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL);  
      angle       = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL); 
      curvature   = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL);  
      follicle_x  = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL);  
      follicle_y  = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL); 
      tip_x       = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL); 
      tip_y       = mxCreateNumericMatrix(1,1,mxDOUBLE_CLASS,mxREAL); 

      if(!( fid        && 
            wid        && 
            label      && 
            face_x     && 
            face_y     && 
            length     && 
            score      && 
            angle      && 
            curvature  && 
            follicle_x && 
            follicle_y && 
            tip_x      && 
            tip_y))
        goto AllocRowException;
       
      mxSetFieldByNumber(plhs[0],i,0, fid       );
      mxSetFieldByNumber(plhs[0],i,1, wid       );
      mxSetFieldByNumber(plhs[0],i,2, label     );
      mxSetFieldByNumber(plhs[0],i,3, face_x    );
      mxSetFieldByNumber(plhs[0],i,4, face_y    );
      mxSetFieldByNumber(plhs[0],i,5, length    );
      mxSetFieldByNumber(plhs[0],i,6, score     );
      mxSetFieldByNumber(plhs[0],i,7, angle     );
      mxSetFieldByNumber(plhs[0],i,8, curvature );
      mxSetFieldByNumber(plhs[0],i,9, follicle_x);
      mxSetFieldByNumber(plhs[0],i,10,follicle_y);
      mxSetFieldByNumber(plhs[0],i,11,tip_x     );
      mxSetFieldByNumber(plhs[0],i,12,tip_y     );
      
      pfid         = (   int*) mxGetData(fid       );
      pwid         = (   int*) mxGetData(wid       );
      plabel       = (   int*) mxGetData(label     );
      pface_x      = (   int*) mxGetData(face_x    );
      pface_y      = (   int*) mxGetData(face_y    );
      plength      = (double*) mxGetData(length    );
      pscore       = (double*) mxGetData(score     );
      pangle       = (double*) mxGetData(angle     ); 
      pcurvature   = (double*) mxGetData(curvature ); 
      pfollicle_x  = (double*) mxGetData(follicle_x);
      pfollicle_y  = (double*) mxGetData(follicle_y);
      ptip_x       = (double*) mxGetData(tip_x     );
      ptip_y       = (double*) mxGetData(tip_y     );

      *pfid = table[i].fid;
      *pwid = table[i].wid;
      *plabel  = table[i].state;
      *pface_x = table[i].face_x;
      *pface_y = table[i].face_y;
      *plength      = table[i].data[0]; //see end of measure.c:Whisker_Segments_Measure for column assignments
      *pscore       = table[i].data[1];
      *pangle       = table[i].data[2];
      *pcurvature   = table[i].data[3];
      *pfollicle_x  = table[i].data[4]; 
      *pfollicle_y  = table[i].data[5]; 
      *ptip_x       = table[i].data[6]; 
      *ptip_y       = table[i].data[7]; 
    }
  }

Done:
  Free_Measurements_Table(table);
  return;
AllocStructArrayException:
  Free_Measurements_Table(table);
  mexPrintf("Allocating struct array for %d rows.\n",nrows);
  mexErrMsgTxt("Allocation failed.\n");
  return;
AllocRowException:
  Free_Measurements_Table(table);
  mexErrMsgTxt("Allocation failed.\n");
  return;

}
#endif


#ifdef SAVE_MEASUREMENTS
/**
 * mexFunction for Matlab function SaveMeasurements.
 *
 * Matlab usage:
 *
 *  Build: (also see buildmex.m)
 *
 *    mex -DSAVE_MEASUREMENTS -o SaveMeasurements measurements_io.mex -lwhisk -L./lib -I./include 
 *
 *  Calling from Matlab console:
 *
 *    SaveMeasurements('path/to/file.measurements',table);
 *
 *  `table` must be a `struct array` with the following fields:
 *    
 *    fid                         - Video frame where the segment was found
 *    wid                         - id for the segment on that frame
 *    label                       - identity.  For tracking use -1 for `not a whisker` and 
 *                                  0,1,2.. for whiskers.                                  
 *    face_x                      - roughly, center of whisker pad, y coordinate           
 *    face_y                      - roughly, center of whisker pad, y coordinate           
 *    length                      - path length in pixels                                  
 *    score                       - median score from tracing                              
 *    follicle_x                  - follicle position: x (px)
 *    follicle_y                  - follicle position: y (px)
 *    tip_x                       - tip position: x (px)
 *    tip_y                       - tip position: y (px)
 *
 *  Notes:
 *
 *    `fid` and `wid` are used as indices to look up the corresponding shape in
 *    a .whiskers file.  Make sure you know what you're doing before changing
 *    these.
 *
 */
#include <matrix.h>
#include <mex.h>
#include "measurements_io.h"

#define mxerror(msg) mexErrMsgTxt((msg))
#define mxassert(expr,msg) if(!(expr)) mxerror((msg))

const char *fields[] = {
                      "fid",
                      "wid",
                      "label",
                      "face_x",
                      "face_y",
                      "length",
                      "score",
                      "angle",
                      "curvature",
                      "follicle_x",
                      "follicle_y",
                      "tip_x",
                      "tip_y"
                      };

void assertFieldNames(const mxArray *s)
{ int i,n;

  n = sizeof(fields)/sizeof(char*);
  mxassert(n<=mxGetNumberOfFields(s),"Got wrong number of fields.\n");
  for(i=0;i<n;++i)
    if(strcmp(fields[i],mxGetFieldNameByNumber(s,i))!=0)
    { mexPrintf("Expected field %d to have name %s.  Got %s.\n",i,fields[i],mxGetFieldNameByNumber(s,i));
      mxerror("Could not interpret struct array.\n");
    }
}
#define countof(e) (sizeof(e)/sizeof(*e))
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ size_t len;
  char *filename;
  Measurements *table;
  int nrows;

  mxassert(nrhs==2             , "Two inputs required.");
  mxassert(nlhs==0             , "Too many output arguments.");
  mxassert(mxIsChar(prhs[0])   , "First input must be a string.");
  mxassert(mxIsStruct(prhs[1]) , "Second input must be a struct.");

  assertFieldNames(prhs[1]);

  mxassert(
      filename=mxArrayToString(prhs[0]),
      "Could not convert input to string.");

  nrows = mxGetM(prhs[1])*mxGetN(prhs[1]);
#ifdef DEBUG
  mexPrintf("Counted %d rows.\n",nrows);
#endif
  mxassert(
      table=Alloc_Measurements_Table(nrows,8), //see end of measure.c:Whisker_Segments_Measure for column assignments 
      "Could not allocate temporary Measurements table.");

  // Copy from struct array to fill up the table
  { int i;
    for(i=0;i<nrows;++i)
    { mxArray *fid,*wid,*label,*face_x,*face_y,*length,*score,*angle,*curvature,*follicle_x,*follicle_y,*tip_x,*tip_y;
      int *pfid,*pwid,*plabel,*pface_x,*pface_y;
      double *plength,*pscore,*pangle, *pcurvature,*pfollicle_x,*pfollicle_y, *ptip_x, *ptip_y;
      
      fid         = mxGetFieldByNumber(prhs[1],i,0);
      wid         = mxGetFieldByNumber(prhs[1],i,1); 
      label       = mxGetFieldByNumber(prhs[1],i,2); 
      face_x      = mxGetFieldByNumber(prhs[1],i,3); 
      face_y      = mxGetFieldByNumber(prhs[1],i,4); 
      length      = mxGetFieldByNumber(prhs[1],i,5); 
      score       = mxGetFieldByNumber(prhs[1],i,6); 
      angle       = mxGetFieldByNumber(prhs[1],i,7); 
      curvature   = mxGetFieldByNumber(prhs[1],i,8); 
      follicle_x  = mxGetFieldByNumber(prhs[1],i,9); 
      follicle_y  = mxGetFieldByNumber(prhs[1],i,10); 
      tip_x       = mxGetFieldByNumber(prhs[1],i,11); 
      tip_y       = mxGetFieldByNumber(prhs[1],i,12); 
#ifdef DEBUG
      mexPrintf("fid        :0x%p\n",fid       );
      mexPrintf("wid        :0x%p\n",wid       );
      mexPrintf("label      :0x%p\n",label     );
      mexPrintf("face_x     :0x%p\n",face_x    );
      mexPrintf("face_y     :0x%p\n",face_y    );
      mexPrintf("length     :0x%p\n",length    );
      mexPrintf("score      :0x%p\n",score     );
      mexPrintf("angle      :0x%p\n",angle     );
      mexPrintf("curvature  :0x%p\n",curvature );
      mexPrintf("follicle_x :0x%p\n",follicle_x);
      mexPrintf("follicle_y :0x%p\n",follicle_y);
      mexPrintf("tip_x      :0x%p\n",tip_x     );
      mexPrintf("tip_y      :0x%p\n",tip_y     );
      mexPrintf("%d of %d\n",i,nrows);
#endif

      // Check pointer and dimensions
      { const mxArray *fields[] = {fid,wid,label,face_x,face_y,length,score,angle,curvature,follicle_x,follicle_y,tip_x,tip_y};
        int j;  
        for(j=0;j<countof(fields);++j)
        { if(!fields[j])           goto GetFieldException;
          if(mxGetM(fields[j])!=1) goto FieldSizeException;
          if(mxGetN(fields[j])!=1) goto FieldSizeException;
        }
      }
       
      pfid         = (   int*) mxGetData(fid       );
      pwid         = (   int*) mxGetData(wid       );
      plabel       = (   int*) mxGetData(label     );
      pface_x      = (   int*) mxGetData(face_x    );
      pface_y      = (   int*) mxGetData(face_y    );
      plength      = (double*) mxGetData(length    );
      pscore       = (double*) mxGetData(score     );
      pangle       = (double*) mxGetData(angle     ); 
      pcurvature   = (double*) mxGetData(curvature ); 
      pfollicle_x  = (double*) mxGetData(follicle_x);
      pfollicle_y  = (double*) mxGetData(follicle_y);
      ptip_x       = (double*) mxGetData(tip_x     );
      ptip_y       = (double*) mxGetData(tip_y     );
      
      //see end of measure.c:Whisker_Segments_Measure for column assignments
      table[i].row            = i;
      table[i].fid            = *pfid;
      table[i].wid            = *pwid;
      table[i].state          = *plabel;
      table[i].face_x         = *pface_x;
      table[i].face_y         = *pface_y;
      table[i].col_follicle_x = 4;
      table[i].col_follicle_y = 5;
      table[i].valid_velocity = 0;
      table[i].data[0]        = *plength;
      table[i].data[1]        = *pscore;
      table[i].data[2]        = *pangle;
      table[i].data[3]        = *pcurvature;
      table[i].data[4]        = *pfollicle_x;
      table[i].data[5]        = *pfollicle_y;
      table[i].data[6]        = *ptip_x;
      table[i].data[7]        = *ptip_y;
    }
  }

//Write
Measurements_Table_To_Filename(filename,NULL,table,nrows);

Done:
  Free_Measurements_Table(table);
  return;
FieldSizeException:  
  Free_Measurements_Table(table);
  mexErrMsgTxt("A field had the wrong dimensions. Aborting without saving.");
  return;
GetFieldException:
  Free_Measurements_Table(table);
  mexErrMsgTxt("Failed to retrieve values from one or more fields.  Aborting without saving.");
  return;

}
#endif
