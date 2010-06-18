/* Author: Nathan Clack
 *   Date: June 14, 2010
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html). 
 *
 *
 * LoadWhiskers
 * ============
 * A matlab interface for reading .whisker files.
 * 
 *   This uses the file format interface defined in whisker_io.h to read a .whisker file into
 *   a MATLAB struct array.
 *
 *   Build
 *   -----
 *   From a MATLAB prompt use:
 *
 *   >> mex -o LoadWhiskers whisker_io.mex.c -lwhisk -L./
 *
 *   or, using the provided M-file: 
 *
 *   >> buildmex
 *
 *   Example:
 *   -------
 *   From inside MATLAB:
 *
 *     >> w = LoadWhiskers('/Users/clackn/Desktop/whisker-mp4/whisker_data_0250.whiskers') 
 *     Got /Users/clackn/Desktop/whisker-mp4/whisker_data_0250.whiskers                    
 *                                                                                        
 *     w =                                                                                 
 *                                                                                        
 *     27771x1 struct array with fields:                                                   
 *         id                                                                              
 *         time                                                                            
 *         x                                                                               
 *         y                                                                               
 *         thick                                                                           
 *         scores                                                                          
 *
 *   If the file doesn't exist or something goes wrong, you'll get a bunch of warnings:
 *
 *     >> LoadWhiskers('does-not-exist.whiskers')                                                       
 *     --- Warning: In is_file_whisk1, could not open file (test.whiskers) for reading.    
 *     --- Warning: Could not open file (test.whiskers) for reading.                       
 *     --- Warning: Could not open file (test.whiskers) for reading.                       
 *     --- Warning: Could not detect whisker file format for test.whiskers.                
 *         Perhaps it's not a whiskers file.                                               
 *     Got test.whiskers                                                                   
 *     ??? Error using ==> LoadWhiskers                                                    
 *     Could not load whiskers.                                                            
 *                                                                                        
 *                                                                                        
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 **/
#include <matrix.h>
#include <mex.h>
#include "whisker_io.h"
#include "trace.h"
#include <string.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ size_t buflen; 
  int nwhisk;
  char *filename;
  Whisker_Seg *ws;
  
  /* check for proper number of arguments */
  if(nrhs!=1) 
    mexErrMsgTxt("One input required.");
  else if(nlhs > 1) 
    mexErrMsgTxt("Too many output arguments.");

  /* input must be a string */
  if ( mxIsChar(prhs[0]) != 1)
    mexErrMsgTxt("Input must be a string.");

  /* get the length of the input string */
  buflen = (mxGetM(prhs[0]) * mxGetN(prhs[0])) + 1;

  /* copy the string data from prhs[0] into a C string input_ buf.
   * */
  filename = mxArrayToString(prhs[0]);

  if(filename == NULL) 
    mexErrMsgTxt("Could not convert input to string.");

  //mexPrintf("Got %s\n",filename);
  
  ws = Load_Whiskers(filename,NULL,&nwhisk);
  if(!ws)
    mexErrMsgTxt("Could not load whiskers.\n");
  
  if(nlhs==0)
    goto Done;
  
  { const char *fields[] = {"id","time","x","y","thick","scores"};        
    plhs[0] = mxCreateStructMatrix(
              nwhisk
            , 1
            , sizeof(fields)/sizeof(char*)
            , fields);
    if(!plhs[0])
    { Free_Whisker_Seg_Vec(ws,nwhisk);
      mexPrintf("Allocating struct array for %d whiskers.\n",nwhisk);
      mexErrMsgTxt("Allocation failed.\n");
    }
  }
    
  { int i;
    for(i=0;i<nwhisk;++i)
    { mxArray *id,*time,*x,*y,*thick,*scores;
      int *pid,*ptime;
      float *px, *py, *pthick, *pscores;
      int n;
      
      n = ws[i].len;
      
      
      id     = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL);
      time   = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL);
      x      = mxCreateNumericMatrix(n,1,mxSINGLE_CLASS,mxREAL);
      y      = mxCreateNumericMatrix(n,1,mxSINGLE_CLASS,mxREAL);
      thick  = mxCreateNumericMatrix(n,1,mxSINGLE_CLASS,mxREAL);
      scores = mxCreateNumericMatrix(n,1,mxSINGLE_CLASS,mxREAL);
      if(!(id && time && x && y && thick && scores))
      { Free_Whisker_Seg_Vec(ws,nwhisk);        
        mexPrintf("Allocating space for whisker #%d (%d points).\n", i, ws[i].len);
        mexErrMsgTxt("Allocation failed.\n");
        return;
      }
      
      mxSetFieldByNumber(plhs[0],i,0,id);
      mxSetFieldByNumber(plhs[0],i,1,time);
      mxSetFieldByNumber(plhs[0],i,2,x);
      mxSetFieldByNumber(plhs[0],i,3,y);
      mxSetFieldByNumber(plhs[0],i,4,thick);
      mxSetFieldByNumber(plhs[0],i,5,scores);
      
      pid = (int*) mxGetData(id);
      *pid = ws[i].id;
      
      ptime = (int*) mxGetData(time);
      *ptime = ws[i].time;
      
      memcpy(mxGetData(x),ws[i].x,n*sizeof(float));
      memcpy(mxGetData(y),ws[i].y,n*sizeof(float));
      memcpy(mxGetData(thick),ws[i].thick,n*sizeof(float));
      memcpy(mxGetData(scores),ws[i].scores,n*sizeof(float));
      
    }
  }
              
Done:
  Free_Whisker_Seg_Vec(ws,nwhisk);
}
