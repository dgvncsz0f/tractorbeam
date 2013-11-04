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
#include "tractorbeam/zkrecv.h"
#include "tractorbeam/zksend.h"
#include "tractorbeam/helpers.h"
#include "tractorbeam/monitor.h"

#define TB_DEFAULT_ENDPOINT "localhost:2181"
#define TB_DEFAULT_TIMEOUT 5000
#define TB_DEFAULT_DELAY 5
#define TB_RECV_BUFSIZE 2097152

static
int __tractorbeam_check_send(tractorbeam_zksend_t *sendcfg)
{
  int rc = 0;
  if (sendcfg->endpoint == NULL || strcmp("", sendcfg->endpoint) == 0)
  {
    printf("ERROR: zookeeper must not be null\n");
    rc = 1;
  }

  if (sendcfg->path == NULL || strcmp("", sendcfg->path) == 0)
  {
    printf("ERROR: path must not be null\n");
    rc = 1;
  }

  if (sendcfg->exec == NULL || strcmp("", sendcfg->exec) == 0)
  {
    printf("ERROR: exec must not be null\n");
    rc = 1;
  }

  if (sendcfg->timeout <= 0)
  {
    printf("ERROR: timeout must be >0\n");
    rc = 1;
  }

  return(rc);
}

static
int __tractorbeam_check_recv(tractorbeam_zkrecv_t *recvcfg)
{
  int rc = 0;

  if (recvcfg->path == NULL || strcmp("", recvcfg->path) == 0)
  {
    printf("ERROR: path must not be null\n");
    rc = 1;
  }

  return(rc);
}

static
size_t __tractorbeam_strlen1(const char *s)
{
  size_t maxlen = 0;
  size_t curlen = 0;
  for (; s[0] != '\0'; s++)
  {
    curlen += 1;
    if (s[0] == '\n')
    {
      maxlen = (curlen > maxlen) ? curlen : maxlen;
      curlen = 0;
    }
  }

  return(curlen > maxlen ? curlen : maxlen);
}

static
void __printf_indent(const char *a, const char *b, size_t maxlen)
{
  char *tmp     = strdup(b);
  char *delim   = " ";
  char *token   = NULL;
  size_t prefix = __tractorbeam_strlen1(a);
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
void __tractorbeam_print_usage0(const char *prg)
{
  printf("USAGE: %s {send,recv} OPTIONS...\n\n", prg);
  printf("  tip: use --help after the sub-comamnd to get a list of available options\n");
}

static
void __tractorbeam_print_sendusage(const char *prg)
{
  char buffer[1024];
  printf("USAGE: %s send OPTIONS... -- [ARGV...]\n", prg);

  __printf_indent("", "  This program connects to a zookeeper cluster and regularly writes"
                      "  the output of a given program into a ephemeral node.", 60);

  snprintf(buffer, 1024, "The zookeeper cluster to connect to [default:%s];", TB_DEFAULT_ENDPOINT);
  __printf_indent("  --zookeeper STRING  ", buffer, 76);

  snprintf(buffer, 1024, "The path of the ephemeral node you want to create;");
  __printf_indent("  --path STRING       ", buffer, 76);

  snprintf(buffer, 1024, "The image to invoke. This should be the an absolute path (but it is"
                         " not enforced);");
  __printf_indent("  --exec FILE         ", buffer, 76);

  snprintf(buffer, 1024, "Defines the interval at which the program gets called. The amount of"
                         " time the program spent during its thing is not taken into account,"
                         " but the runtime may not exceed this value [default:%d];", TB_DEFAULT_DELAY);
  __printf_indent("  --delay SECONDS     ", buffer, 76);

  snprintf(buffer, 1024, "This defines how much time without communication zookeeper should"
                         " consider the client still alive [default:%d];", TB_DEFAULT_TIMEOUT);
  __printf_indent("  --timeout MILLISECS ", buffer, 76);

}

static
void __tractorbeam_print_recvusage(const char *prg)
{
  char buffer[1024];
  printf("USAGE: %s recv OPTIONS...\n", prg);

  __printf_indent("", "  This program connects to a zookeeper cluster and reads its"
                      "  entire tree allowing the user to dump the contents in many"
                      "  different formats.", 60);

  snprintf(buffer, 1024, "The zookeeper cluster to connect to [default:%s];", TB_DEFAULT_ENDPOINT);
  __printf_indent("  --zookeeper STRING  ", buffer, 76);

  snprintf(buffer, 1024, "The tree you want to read;");
  __printf_indent("  --path STRING       ", buffer, 76);

  snprintf(buffer, 1024, "The file to write the contents. Use - to write into the stdout [default:-];");
  __printf_indent("  --output FILE       ", buffer, 76);
}

static
int __tractorbeam_parse_recvopts(int argc, char *argv[], tractorbeam_zkrecv_t *recvcfg)
{
  static struct option my_options[] = {
    {"zookeeper",     required_argument, NULL, 0 },
    {"path",          required_argument, NULL, 0 },
    {"output",        required_argument, NULL, 0 },
    {"help",          no_argument,       NULL, 0 },
    {0,               0,                 NULL, 0 }
  };

  while (1)
  {
    int opt = 0;
    int rc  = getopt_long_only(argc, argv, "", my_options, &opt);
    if (rc == -1)
    { break; }
    else if (rc == 0)
    {
      if (opt == 0)
      { recvcfg->endpoint = optarg; }
      else if (opt == 1)
      { recvcfg->path = optarg; }
      else if (opt == 2)
      {
        if (strcmp(optarg, "-") == 0)
        { recvcfg->file = stdout; }
        else
        { recvcfg->file = fopen(optarg, "w"); }
      }
      else
      { return(-1); }
    }
    else
    { return(-1); }
  }

  return(__tractorbeam_check_recv(recvcfg));
}

static
int __tractorbeam_parse_sendopts(int argc, char *argv[], tractorbeam_zksend_t *sendcfg)
{
  static struct option my_options[] = {
    {"zookeeper",     required_argument, NULL, 0 },
    {"path",          required_argument, NULL, 0 },
    {"exec",          required_argument, NULL, 0 },
    {"timeout",       required_argument, NULL, 0 },
    {"delay",         required_argument, NULL, 0 },
    {"help",          no_argument,       NULL, 0 },
    {0,               0,                 NULL, 0 }
  };

  while (1)
  {
    int opt = 0;
    int rc  = getopt_long_only(argc, argv, "", my_options, &opt);
    if (rc == -1)
    { break; }
    else if (rc == 0)
    {
      if (opt == 0)
      { sendcfg->endpoint = optarg; }
      else if (opt == 1)
      { sendcfg->path = optarg; }
      else if (opt == 2)
      { sendcfg->exec = optarg; }
      else if (opt == 3)
      { sendcfg->timeout = atoi(optarg); }
      else if (opt == 4)
      { sendcfg->delay = atoi(optarg); }
      else
      { return(-1); }
    }
    else
    { return(-1); }
  }

  int k            = optind;
  sendcfg->argv    = (char **) malloc(sizeof(char*) * (2 + (argc - optind)));
  sendcfg->argv[0] = sendcfg->exec;
  for (; k<argc; k+=1)
  { sendcfg->argv[1+k-optind] = argv[k]; }
  sendcfg->argv[1+k-optind] = NULL;

  return(__tractorbeam_check_send(sendcfg));
}

int main(int argc, char *argv[])
{
  tractorbeam_zksend_t sendcfg;
  sendcfg.endpoint  = TB_DEFAULT_ENDPOINT;
  sendcfg.path      = "";
  sendcfg.exec      = "";
  sendcfg.argv      = NULL;
  sendcfg.delay     = TB_DEFAULT_DELAY;
  sendcfg.timeout   = TB_DEFAULT_TIMEOUT;

  tractorbeam_zkrecv_t recvcfg;
  recvcfg.endpoint  = TB_DEFAULT_ENDPOINT;
  recvcfg.path      = "";
  recvcfg.file      = stdout;
  recvcfg.delay     = TB_DEFAULT_DELAY;
  recvcfg.timeout   = TB_DEFAULT_TIMEOUT;

  if (argc < 2)
  {
    __tractorbeam_print_usage0(argv[0]);
    return(-1);
  }

  if (strcmp(argv[1], "send") == 0)
  {
    argv[1] = argv[0];
    if (__tractorbeam_parse_sendopts(argc-1, argv+1, &sendcfg) != 0)
    {
      __tractorbeam_print_sendusage(argv[0]);
      return(-1);
    }

    int rc = tractorbeam_zksend(&sendcfg);
    free(sendcfg.argv);
    return(rc);
  }
  else if (strcmp(argv[1], "recv") == 0)
  {
    argv[1] = argv[0];
    if (__tractorbeam_parse_recvopts(argc-1, argv+1, &recvcfg) != 0)
    {
      __tractorbeam_print_recvusage(argv[0]);
      return(-1);
    }

    return(tractorbeam_zkrecv(&recvcfg));
  }
  else
  {
    __tractorbeam_print_usage0(argv[0]);
    return(-1);
  }

  return(-1);
}
