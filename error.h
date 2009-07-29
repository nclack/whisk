#ifndef _H_ERROR_REPORT
#define _H_ERROR_REPORT

#define SHOW_DEBUG_MESSAGES     1
#define SHOW_PROGRESS_MESSAGES  1  

void error(char *str, ... );
void warning(char *str, ... );
void debug(char *str, ... );
void progress(char *str, ... );
void progress_meter(double cur, double min, double max, int len, char *str, ...);

#endif//_H_ERROR_REPORT
