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

#define _POSIX_C_SOURCE 1

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "tractorbeam/debug.h"
#include "tractorbeam/popen.h"

extern char **environ;

struct tractorbeam_popen_t
{
  int fd;
  pid_t pid;
};

tractorbeam_popen_t *tractorbeam_popen_init(const char *prg, char * const *argv, char * const *envv)
{
  int comm[2]             = {-1, -1};
  tractorbeam_popen_t *ph = (tractorbeam_popen_t *) malloc(sizeof(tractorbeam_popen_t));
  if (ph == NULL)
  { return(NULL); }

  if (pipe(comm) != 0)
  { goto handle_error; }

  pid_t pid = fork();
  if (pid == -1)
  { goto handle_error; }

  if (pid == 0)
  {
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    { _exit(EXIT_FAILURE); }

    close(comm[0]);
    if (STDIN_FILENO != comm[1])
    {
      if (dup2(comm[1], STDOUT_FILENO) == -1)
      { _exit(EXIT_FAILURE); }
    }

    if (dup2(fd, STDIN_FILENO) == -1)
    { _exit(EXIT_FAILURE); }

    if (dup2(fd, STDERR_FILENO) == -1)
    { _exit(EXIT_FAILURE); }

    execve(prg, argv, (envv == NULL) ? environ : envv);
    _exit(EXIT_FAILURE);
  }
  else
  {
    close(comm[1]);
    ph->fd  = comm[0];
    ph->pid = pid;
  }

  return(ph);

handle_error:
  if (comm[0] != -1)
  { close(comm[0]); }
  if (comm[1] != -1)
  { close(comm[1]); }
  free(ph);
  return(NULL);
}

int tractorbeam_popen_fd(tractorbeam_popen_t *ph)
{ return(ph->fd); }

int tractorbeam_popen_term(tractorbeam_popen_t *ph, int timeout)
{
  pid_t c_pid = ph->pid;
  int status  = -1;
  close(ph->fd);
  free(ph);

  pid_t t_pid = fork();
  if (t_pid == -1)
  { kill(c_pid, SIGKILL); }

  if (t_pid == 0)
  {
    if (kill(c_pid, 0) == 0)
    {
      if (timeout > 0)
      { sleep(timeout); }
      kill(c_pid, SIGKILL);
    }
    _exit(EXIT_SUCCESS);
  }
  else
  {
    if (waitpid(c_pid, &status, 0) != c_pid)
    { status = -1; }
    if (t_pid != -1)
    {
      kill(t_pid, SIGTERM);
      waitpid(t_pid, NULL, 0);
    }
  }
  return(status);
}
