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

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include "tractorbeam/exec.h"
#include "tractorbeam/debug.h"
#include "tractorbeam/popen.h"

static
size_t __tbexec_read(int fd, int timeout_in_sec, char *out, size_t outsz)
{
  char buffer[TRACTORBEAM_BUFFER_SIZE];
  fd_set r_set;
  size_t offset;
  struct timeval timeout;

  offset          = 0;
  timeout.tv_sec  = timeout_in_sec;
  timeout.tv_usec = 0;

  FD_ZERO(&r_set);
  FD_SET(fd, &r_set);
  while (1)
  {
    int rc = select(fd+1, &r_set, NULL, NULL, &timeout);
    if (rc == -1)
    { return(offset = -1); }

    if (timeout.tv_sec <= 0)
    { return(-2); }

    if (FD_ISSET(fd, &r_set))
    {
      size_t rc = read(fd, buffer, TRACTORBEAM_BUFFER_SIZE);
      if (rc == 0)
      { break; }
      else if (rc == (size_t) -1)
      { return(-1); }
      else
      {
        if (outsz > rc)
        {
          memcpy(out+offset, buffer, rc);
          outsz  -= rc;
          offset += rc;
        }
        else
        { return(-3); }
      }
    }
  }

  return(offset);
}

int tractorbeam_exec(const char *prg, char * const * argv, int timeout, int *estatus, void *out, size_t outsz)
{
  tractorbeam_popen_t *proc = tractorbeam_popen_init(prg, argv, NULL);
  if (proc == NULL)
  { return(-1) ; }

  size_t offset = __tbexec_read(tractorbeam_popen_fd(proc), timeout, out, outsz);

  *estatus = tractorbeam_popen_term(proc, 1);
  return((int) offset);
}
