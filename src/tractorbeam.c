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
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string.h>
#include "tractorbeam/debug.h"
#include "tractorbeam/tbloop.h"
#include "tractorbeam/helpers.h"
#include "tractorbeam/monitor.h"

#define TB_DEFAULT_ENDPOINT "localhost:2181"
#define TB_DEFAULT_TIMEOUT 5000
#define TB_DEFAULT_DELAY 5

static
int __tractorbeam_check(tractorbeam_runtime_t *optval)
{
  int rc = 0;
  if (optval->endpoint == NULL || strcmp("", optval->endpoint) == 0)
  {
    printf("ERROR: zookeeper must not be null\n");
    rc = 1;
  }

  if (optval->path == NULL || strcmp("", optval->path) == 0)
  {
    printf("ERROR: path must not be null\n");
    rc = 1;
  }

  if (optval->exec == NULL || strcmp("", optval->exec) == 0)
  {
    printf("ERROR: exec must not be null\n");
    rc = 1;
  }

  if (optval->timeout <= 0)
  {
    printf("ERROR: timeout must be >0\n");
    rc = 1;
  }

  return(rc);
}

static
void __printf_indent(const char *a, const char *b, size_t maxlen)
{
  char *tmp     = strdup(b);
  char *delim   = " ";
  char *token   = NULL;
  size_t prefix = strlen(a);
  size_t at     = 0;

  printf("\n%s", a);
  token = strtok(tmp, delim);
  do
  {
    at += strlen(token);
    if ((at+prefix > maxlen) || (strcmp("\n", token) == 0))
    {
      at = prefix;
      printf("\n");
      while (at-- > 0)
      { printf(delim); }
      at = 0;
    }
    if (strcmp("\n", token) != 0)
    { printf("%s ", token); }
  } while ((token = strtok(NULL, delim)) != NULL);
  printf("\n");

  free(tmp);
}

static
void __tractorbeam_print_usage(const char *prg)
{
  char buffer[1024];
  printf("USAGE: %s OPTIONS... -- [ARGV...]\n", prg);

  __printf_indent("", "  This program connects to a zookeeper cluster and regularly writes"
                      "  the output of a given program into a ephemeral node.", 60);

  snprintf(buffer, 1024, "The zookeeper cluster to connect to [default: %s].", TB_DEFAULT_ENDPOINT);
  __printf_indent("  --zookeeper STRING  ", buffer, 76);

  snprintf(buffer, 1024, "The path of the ephemeral node you want to create [mandatory]");
  __printf_indent("  --path STRING       ", buffer, 76);

  snprintf(buffer, 1024, "The image to invoke. This should be the an absolute path (but it is"
                         "not enforced) [mandatory]");
  __printf_indent("  --exec FILE         ", buffer, 76);

  snprintf(buffer, 1024, "Defines the interval at which the program gets called. The amount of"
                         " time the program spent during its thing is not taken into account,"
                         " but the runtime may not exceed this value. [default:%d]", TB_DEFAULT_DELAY);
  __printf_indent("  --delay-in-s INT    ", buffer, 76);

  snprintf(buffer, 1024, "Timeout in milliseconds. This defines how much time without"
                         " communication zookeeper should consider the client still alive."
                         " [default:%d]", TB_DEFAULT_TIMEOUT);
  __printf_indent("  --timeout-in-ms INT ", buffer, 76);

}

int main(int argc, char *argv[])
{
  tractorbeam_runtime_t optvals;
  optvals.endpoint  = TB_DEFAULT_ENDPOINT;
  optvals.path      = "";
  optvals.exec      = "";
  optvals.argv      = NULL;
  optvals.delay     = TB_DEFAULT_DELAY;
  optvals.timeout   = TB_DEFAULT_TIMEOUT;

  static struct option tb_options[] = {
    {"zookeeper",     required_argument, NULL, 0 },
    {"path",          required_argument, NULL, 0 },
    {"exec",          required_argument, NULL, 0 },
    {"timeout-in-ms", required_argument, NULL, 0 },
    {"delay-in-s",    required_argument, NULL, 0 },
    {"help",          no_argument,       NULL, 0 },
    {0,               0,                 NULL, 0 }
  };

  while (1)
  {
    int opt = 0;
    int rc  = getopt_long_only(argc, argv, "", tb_options, &opt);
    if (rc == -1)
    { break; }
    else if (rc == 0)
    {
      if (opt == 0)
      { optvals.endpoint = optarg; }
      else if (opt == 1)
      { optvals.path = optarg; }
      else if (opt == 2)
      { optvals.exec = optarg; }
      else if (opt == 3)
      { optvals.timeout = atoi(optarg); }
      else if (opt == 4)
      { optvals.delay = atoi(optarg); }
      else
      {
        __tractorbeam_print_usage(argv[0]);
        return(0);
      }
    }
    else
    {
      __tractorbeam_print_usage(argv[0]);
      return(-1);
    }
  }

  int k           = optind;
  optvals.argv    = (char **) malloc(sizeof(char*) * (2 + (argc - optind)));
  optvals.argv[0] = optvals.exec;
  for (; k<argc; k+=1)
  { optvals.argv[1+k-optind] = argv[k]; }
  optvals.argv[1+k-optind] = NULL;

  if (__tractorbeam_check(&optvals) != 0)
  { return(1); }

  int rc = tractorbeam_loop(&optvals);
  free(optvals.argv);
  return(rc);
}
