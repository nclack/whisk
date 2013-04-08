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
 *   >> mex -o LoadWhiskers whisker_io.mex.c -lwhisk -L<path_to_whisk_library> -I<path_to_whisk_includes>
 *
 *   For example, with everything installed to C:\WhiskerTracker
 *
 *   >> mex -o LoadWhiskers whisker_io.mex.c -lwhisk -L'C:\WhiskerTracker' -I'C:\WhiskerTracker' 
 *
 *   On windows it may be necessary to add 
 * 
 *      LINKFLAGSPOST=/NODEFAULTLIB:LIBCMT
 *
 *   to the mex command line or to your mexopts.bat.
 *
 *   Usage:
 *   -----
 *
 *   whiskers          = LoadWhiskers(filename)
 *   [whiskers,format] = LoadWhiskers(filename)
 *
 *   <whiskers> is a struct array with a number of fields describing the shape of traced whisker-like segments.
 *   <format>   is a string describing how the whisker data is stored.
 *
 *   Example:
 *   -------
 *   From inside MATLAB:
 *
 *     >> w = LoadWhiskers('/Users/clackn/Desktop/whisker-mp4/whisker_data_0250.whiskers') 
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
 **/
#include <matrix.h>
#include <mex.h>
#include "whisker_io.h"
#include "trace.h"
#include <string.h>

#include <stdarg.h>
#define REPORT(msg1,msg2) mexErrMsgIdAndTxt("WhiskerTracker:internal","%s(%d) - %s()\n\t%s\n\t%s\n",__FILE__,__LINE__,__FUNCTION__,msg1,msg2)
#define TRY(e) do{ if(!(e)) { REPORT(#e,"Expression evaluated as false."); goto Error;}} while(0)
#define NEW(T,e,N) TRY((e)=(T*)calloc((N),sizeof(T)))

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

#ifdef LOAD_WHISKERS
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ size_t buflen; 
  int nwhisk;
  char *filename;
  Whisker_Seg *ws;

  set_reporter(mxreport);
  
  /* check for proper number of arguments */
  if(nrhs!=1) 
    mexErrMsgTxt("One input required.");
  else if(nlhs > 2) 
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
  
  if(nlhs==2)
  { char *format=NULL;
    if( Whisker_File_Autodetect(filename,&format)==-1 ) 
      goto AutodetectException;
  
    plhs[1] = mxCreateString(format);
    if(!plhs[1])
      goto AllocateFormatStringException;
  }
  
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
  return;
AutodetectException:
  Free_Whisker_Seg_Vec(ws,nwhisk);
  mexErrMsgTxt("Could not autodetect file format.\n");
AllocateFormatStringException:
  Free_Whisker_Seg_Vec(ws,nwhisk);
  mexErrMsgTxt("Allocation failed for format string.\n");
}
#endif

/*
 * mexSaveWhiskers
 * ============
 *
 * Usage:
 *   ecode = mexSaveWhiskers(filename,w)
 *
 * 
 * 
 */
#ifdef SAVE_WHISKERS

static char* get_filename(const mxArray *arg)
{ 
  if ( mxIsChar(arg) != 1) /* input must be a string */
  { mexErrMsgTxt("Input must be a string.");
    return 0;
  }
  //buflen = (mxGetM(arg) * mxGetN(arg)) + 1; /* get the length of the input string */
  return mxArrayToString(arg); /* copy the string data from arg into a C string input_ buf. */
}

static int get_whiskers(const mxArray *arg, Whisker_Seg **ws_, int *n_)
{ size_t n,i;
  Whisker_Seg *ws=0;
  int ok=0;
  // validate arg as an appropriate struct
  TRY(mxIsStruct(arg));
  TRY((n=mxGetNumberOfElements(arg))>0);
  NEW(Whisker_Seg,ws,n);
  for(i=0;i<n;++i)
  { mxArray *id=0,*time=0,*x=0,*y=0,*thick=0,*score=0;
    size_t len=0;    

    TRY(id=mxGetField(arg,i,"id"));
    TRY(time=mxGetField(arg,i,"time"));
    TRY(x=mxGetField(arg,i,"x"));
    TRY(y=mxGetField(arg,i,"y"));
    TRY(thick=mxGetField(arg,i,"thick"));
    TRY(score=mxGetField(arg,i,"scores"));

    TRY(mxGetClassID(id)  ==mxINT32_CLASS);
    TRY(mxGetClassID(time)==mxINT32_CLASS);
    TRY(mxIsSingle(x));
    TRY(mxIsSingle(y));
    TRY(mxIsSingle(score));
    TRY(mxIsSingle(thick));

    TRY(len=mxGetNumberOfElements(x));
    TRY(mxGetNumberOfElements(y     )==len);
    TRY(mxGetNumberOfElements(score )==len);
    TRY(mxGetNumberOfElements(thick )==len);
    NEW(float,ws[i].x     ,len);
    NEW(float,ws[i].y     ,len);
    NEW(float,ws[i].scores,len);
    NEW(float,ws[i].thick ,len);
    
    ws[i].id  =*(int*)mxGetData(id);
    ws[i].time=*(int*)mxGetData(time);
    ws[i].len=len;
    memcpy(ws[i].x     ,mxGetData(x)     ,len*sizeof(float));
    memcpy(ws[i].y     ,mxGetData(y)     ,len*sizeof(float));
    memcpy(ws[i].scores,mxGetData(score) ,len*sizeof(float));
    memcpy(ws[i].thick ,mxGetData(thick) ,len*sizeof(float));
  }
  ok=1;
Finalize:
  *ws_=ws;
  *n_ =n;
  return ok;
Error:
  ok=0;
  goto Finalize;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{ size_t buflen; 
  int n;
  char *filename;
  Whisker_Seg *ws=0;
  int ecode=0;

  set_reporter(mxreport);
  
  /* check for proper number of arguments */
  if(nrhs!=2) 
    mexErrMsgTxt("Two inputs required.");
  else if(nlhs > 1) 
    mexErrMsgTxt("Too many output arguments.");

  TRY(filename=get_filename(prhs[0]));
  TRY(get_whiskers(prhs[1],&ws,&n));
  TRY(Save_Whiskers(filename,NULL,ws,n));

Done:
  { plhs[0] = mxCreateNumericMatrix(1,1,mxINT32_CLASS,mxREAL);
    *((int*) mxGetData(plhs[0]))=ecode;
  }
  if(ws)
    Free_Whisker_Seg_Vec(ws,n);
  return;
Error:
  ecode=1;
  goto Done;
}
#endif