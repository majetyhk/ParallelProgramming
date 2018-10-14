#ifndef __LAKE_LOG_H__
#define __LAKE_LOG_H__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

typedef struct {
  char logfile[64];
  FILE *lf;
} _LOGFILE;

_LOGFILE mylog;

char lake_workdir[64];

void set_wrkdir(char *arg0)
{
  int ln_dir = strrchr(arg0,'/')-arg0+1;
  strncpy(lake_workdir, arg0, ln_dir);
  lake_workdir[ln_dir]='\0';
}

void dir_string(char *file, char *full)
{
  sprintf(full, "%s/%s", lake_workdir, file);
}

void start_lake_log(char *lf_name)
{
  dir_string(lf_name, mylog.logfile);
  mylog.lf = fopen(mylog.logfile, "w+");
}

void lake_log(char *msg, ...)
{
  va_list args;
  va_start(args, msg);

  vfprintf(mylog.lf, msg, args);

  va_end(args);
}

void stop_lake_log()
{
  fclose(mylog.lf);
}



#endif
