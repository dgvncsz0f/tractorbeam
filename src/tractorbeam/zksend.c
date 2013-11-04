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
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "tractorbeam/exec.h"
#include "tractorbeam/debug.h"
#include "tractorbeam/zksend.h"
#include "tractorbeam/monitor.h"

#define ZKSEND_BUFSIZE 1048576

static
void __zksend_debug_rt(tractorbeam_zksend_t *rt)
{
  char buffer[4096];
  char **argv  = rt->argv + 1;
  size_t limit = 4096, offset = 0;

  offset += snprintf(buffer, limit, rt->exec);
  while (limit > offset && argv[0] != NULL)
  {
    offset += snprintf(buffer+offset, limit-offset, " %s", argv[0]);
    argv    = argv + 1;
  }
  TB_DEBUG("using: %s", buffer);
}

int tractorbeam_zksend(tractorbeam_zksend_t *rt)
{
  char buffer[ZKSEND_BUFSIZE];
  int status;

  __zksend_debug_rt(rt);
  tractorbeam_monitor_t *mh = tractorbeam_monitor_init(rt->endpoint, rt->path, rt->timeout);
  if (mh == NULL)
  {
    TB_DEBUG0("error connecting to zookeeper");
    return(-1);
  }

  do
  {
    int rc = tractorbeam_exec(rt->exec, rt->argv, rt->delay, &status, buffer, ZKSEND_BUFSIZE);
    if (rc == -2)
    {
      TB_DEBUG("%s: timeout; [removing node]", rt->exec);
      tractorbeam_monitor_delete(mh);
    }
    else if (rc == -3)
    {
      TB_DEBUG("%s: output too large; [removing node]", rt->exec);
      tractorbeam_monitor_delete(mh);
    }
    else if (rc >= 0)
    {
      if (status != 0)
      {
        TB_DEBUG("%s: exit code == %d; [removing node]", rt->exec, status);
        tractorbeam_monitor_delete(mh);
      }
      else
      { tractorbeam_monitor_update(mh, buffer, rc); }
    }
    else
    {
      TB_DEBUG("%s: error running; [removing node]", rt->exec);
      tractorbeam_monitor_delete(mh);
    }

    sleep(rt->delay);
  } while (1);

  tractorbeam_monitor_term(mh);

  return(-1);
}
