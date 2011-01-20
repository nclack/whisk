/* Author: Nathan Clack <clackn@janelia.hhmi.org>
 * Date  : May 2009 
 *
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
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
