#include "param.h"
#include <stdio.h>

int
main(int argc, char* argv[])
{ int sts;
  if(argc==1)
    sts = Load_Params_File("default.parameters");
  else
    sts = Load_Params_File(argv[1]);
  printf("Status: %s\n",(sts?"Failed":"OK"));
  if(sts==0)
    printf("Width step: %f\n",WIDTH_STEP);
  return sts;
}
