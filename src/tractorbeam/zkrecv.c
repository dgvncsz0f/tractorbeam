// All rights reserved.
//  
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//  
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//  
// * Redistributions in binary form must reproduce the above copyright notice, this
//   list of conditions and the following disclaimer in the documentation and/or
//   other materials provided with the distribution.
//  
// * Neither the name of the {organization} nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//  
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "tractorbeam/debug.h"
#include "tractorbeam/zkrecv.h"
#include "tractorbeam/helpers.h"
#include "tractorbeam/monitor.h"

static
int __tbzkrcv_filesystem_cc(tb_snapshot_events event, const char *ppath, const char *name, const void *contents, size_t contsize, void *data)
{
  char *chdir = (char *) data;
  char *dir   = NULL;
  char *file  = NULL;
  FILE *fd    = NULL;
  int rc      = -1;
  if (event == DONE)
  { return(0); }
  else if (event != ITEM)
  { return(-1); }

  dir = tbh_join(chdir, "/", ppath, "/", name, NULL);
  if (dir != NULL)
  { mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); }

  if (contents == 0)
  { rc = 0; }
  else
  {
    file = tbh_join(chdir, "/", ppath, "/", name, ".data", NULL);
    if (file != NULL && (fd = fopen(file, "w")) != NULL)
    {
      if (fwrite(contents, sizeof(char), contsize, fd) > 0)
      { rc = 0; }
      fclose(fd);
    }
    else
    {
      TB_DEBUG("could not open file: %s/%s/%s", chdir, ppath, name);
      rc = -1;
    }
  }
    
  free(dir);
  free(file);
  return(rc);
}

static
int __tbzkrcv_file_cc(tb_snapshot_events event, const char *ppath, const char *name, const void *contents, size_t contsize, void *data)
{
  FILE *file = (FILE *) data;
  if (event == DONE)
  { return(0); }
  else if (event != ITEM)
  { return(-1); }

  if (contsize > 0)
  {
    if (fprintf(file, "%s/%s|%zd\n", ppath, name, contsize) > 0 &&
        fwrite(contents, sizeof(char), contsize, file) > 0 &&
        fprintf(file, "\n") > 0)
    { return(0); }
    return(-1);
  }

  return(0);
}

int tractorbeam_zkrecv(tractorbeam_zkrecv_t *info)
{
  tractorbeam_monitor_t *mh = tractorbeam_monitor_init(info->endpoint, info->path, info->timeout);
  if (mh == NULL)
  {
    TB_DEBUG0("error connecting to zookeeper");
    return(-1);
  }

  int rc = -1;
  if (info->layout == ZKRECV_LAYOUT_FILE)
  {
    int dash   = strcmp(info->output, "-");
    FILE *file = (dash == 0) ? stdout : fopen(info->output, "w");
    if (file != NULL)
    {
      rc = tractorbeam_monitor_snapshot(mh, info->path, __tbzkrcv_file_cc, file);
      if (dash != 0)
      { fclose(file); }
    }
    else
    {
      rc = -1;
      TB_DEBUG("could not open file: %s", info->output);
    }
  }
  else if (info->layout == ZKRECV_LAYOUT_FILESYSTEM)
  {
    mkdir(info->output, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    rc = tractorbeam_monitor_snapshot(mh, info->path, __tbzkrcv_filesystem_cc, info->output);
  }
  tractorbeam_monitor_term(mh);
  return(rc);
}
