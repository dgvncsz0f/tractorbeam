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

#include <libgen.h>
#include <string.h>
#include <pthread.h>
#include <zookeeper/zookeeper.h>
#include "tractorbeam/debug.h"
#include "tractorbeam/helpers.h"
#include "tractorbeam/monitor.h"

struct tractorbeam_monitor_t
{
  zhandle_t *zh;
  int timeout;
  char *znode;
  char *endpoint;
  pthread_mutex_t mutex;
};

static
void __tbm_reconnect(tractorbeam_monitor_t *mh, watcher_fn fn)
{
  if (pthread_mutex_lock(&mh->mutex) != 0)
  { return; }

  if (mh->zh != NULL)
  { zookeeper_close(mh->zh); }
  mh->zh = zookeeper_init(mh->endpoint, fn, mh->timeout, NULL, mh, 0);

  pthread_mutex_unlock(&mh->mutex);
}

static
void __tbm_watcher(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
  UNUSED(zh);
  UNUSED(path);
  tractorbeam_monitor_t *mh = (tractorbeam_monitor_t *) ctx;

  if (type == ZOO_SESSION_EVENT)
  {
    if (state == ZOO_EXPIRED_SESSION_STATE)
    { __tbm_reconnect(mh, __tbm_watcher); }
  }
}

static
int __tbm_zkcreate(tractorbeam_monitor_t *mh, const void *data, size_t datasize)
{
  int rc = zoo_create(mh->zh, mh->znode, data, datasize, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, NULL, 0);
  if (rc == ZNODEEXISTS)
  { return(1); }
  else if (rc == ZNONODE)
  { return(-2); }
  else if (rc == ZOK)
  { return(0); }
  else
  { return(-1); }
}

static
int __tbm_zkupdate(tractorbeam_monitor_t *mh, struct Stat *stat, const void *data, size_t datasize)
{
  int rc = zoo_set(mh->zh, mh->znode, data, datasize, stat->version);
  if (rc == ZNONODE || rc == ZBADVERSION)
  { return(1); }
  else if (rc == ZOK)
  { return(0); }
  else
  { return(-1); }
}

static
int __tbm_zkcheck(tractorbeam_monitor_t *mh, struct Stat *stat)
{
  const clientid_t *client = zoo_client_id(mh->zh);
  if (client == NULL)
  { return(-1); }
  
  if (stat->ephemeralOwner != client->client_id)
  {
    int rc = zoo_delete(mh->zh, mh->znode, stat->version);
    if (rc == ZOK || rc == ZBADVERSION)
    { return(1); }
    else
    { return(-1); }
  }
  else
  { return(0); }
}

static
int __tbm_snapshot(tractorbeam_monitor_t *mh, const char *ppath, const char *name, int *status, char *buffer, size_t bufsize, tb_snapshot_fn callback, void *data)
{
  struct String_vector children;
  int rc, zrc;
  int r_bufsize = (int) bufsize;
  char *path    = tbh_join(ppath, "/", name, NULL);
  if (path == NULL)
  { return(-1); }

  TB_DEBUG("__tbm_snapshot: %s,%s => %s", ppath, name, path);
  rc  = -1;
  zrc = zoo_get_children(mh->zh, path, 0, &children);
  if (zrc != ZOK)
  {
    TB_DEBUG("error listing children of: %s", path);
    goto handle_error;
  }

  rc  = -1;
  zrc = zoo_get(mh->zh, path, 0, buffer, &r_bufsize, NULL);
  if (zrc != ZOK)
  {
    TB_DEBUG("error retrieving contents of: %s/%s", ppath, name);
    goto handle_error;
  }

  rc      = -2;
  *status = callback(ITEM, ppath, name, buffer, (size_t) r_bufsize, data);
  if (*status != 0)
  {
    TB_DEBUG("callback has failed: %s/%s/%d", ppath, name, *status);
    goto handle_error;
  }

  for (int k=0; k<children.count; k+=1)
  {
    rc = __tbm_snapshot(mh, path, children.data[k], status, buffer, bufsize, callback, data);
    if (rc != 0)
    { goto handle_error; }
  }

  free(path);
  return(0);

handle_error:
  free(path);
  return(-1);
}

tractorbeam_monitor_t *tractorbeam_monitor_init(const char *endpoint, const char *znode, int timeout_in_ms)
{
  tractorbeam_monitor_t *mh = (tractorbeam_monitor_t *) malloc(sizeof(tractorbeam_monitor_t));
  if (mh == NULL)
  { goto handle_error; }

  mh->zh       = NULL;
  mh->znode    = NULL;
  mh->timeout  = timeout_in_ms;
  mh->endpoint = NULL;

  if (pthread_mutex_init(&mh->mutex, NULL) != 0)
  {
    free(mh);
    return(NULL);
  }

  mh->znode = tbh_strdup(znode);
  if (mh->znode == NULL)
  { goto handle_error; }

  mh->endpoint = tbh_strdup(endpoint);
  if (mh->endpoint == NULL)
  { goto handle_error; }

  __tbm_reconnect(mh, __tbm_watcher);
  return(mh);

handle_error:
  tractorbeam_monitor_term(mh);
  return(NULL);
}

int tractorbeam_monitor_update(tractorbeam_monitor_t *mh, const void *data, size_t datasize)
{
  struct Stat stat;
  int rc, code = -1;

  if (pthread_mutex_lock(&mh->mutex) != 0)
  { return(-1); }

  if (mh->zh == NULL)
  { code = 1; }
  else
  {
    rc = zoo_exists(mh->zh, mh->znode, 0, &stat);
    if (rc == ZNONODE)
    { code = __tbm_zkcreate(mh, data, datasize); }
    else if (rc == ZOK)
    {
      code = __tbm_zkcheck(mh, &stat);
      if (code == 0)
      { code = __tbm_zkupdate(mh, &stat, data, datasize); }
    }
    else
    { code = -1; }
  }
 
  pthread_mutex_unlock(&mh->mutex);

  return(code);
}

int tractorbeam_monitor_snapshot(tractorbeam_monitor_t *mh, const char *path, tb_snapshot_fn callback, void *data)
{
  if (pthread_mutex_lock(&mh->mutex) != 0)
  { return(-1); }

  int status;
  char *buffer = (char *) malloc(1048576);
  char *path1  = tbh_strdup(path);
  char *path2  = tbh_strdup(path);
  char *ppath  = (path1 == NULL) ? NULL : dirname(path1);
  char *name   = (path2 == NULL) ? NULL : basename(path2);
  int rc       = -1;

  if (ppath != NULL && name != NULL && buffer != NULL)
  {
    if (strcmp(ppath, "/") == 0)
    { rc = __tbm_snapshot(mh, "", name, &status, buffer, 1048576, callback, data); }
    else
    { rc = __tbm_snapshot(mh, ppath, name, &status, buffer, 1048576, callback, data); }
  }
  if (rc == 0)
  { status = callback(DONE, path, "", NULL, 0, data); }
  else if (rc == -1)
  { status = callback(FAIL, path, "", NULL, 0, data); }

  free(buffer);
  free(path1);
  free(path2);

  pthread_mutex_unlock(&mh->mutex);
  return(status);
}

int tractorbeam_monitor_delete(tractorbeam_monitor_t *mh)
{
  int code = -1;
  if (pthread_mutex_lock(&mh->mutex) != 0)
  { return(-1); }

  if (mh->zh == NULL)
  { code = 1; }
  else
  {
    int rc = zoo_delete(mh->zh, mh->znode, -1);
    if (rc == ZOK || rc == ZNONODE)
    { code = 0; }
    else
    { code = -1; }
  }

  pthread_mutex_unlock(&mh->mutex);
  return(code);
}

int tractorbeam_monitor_term(tractorbeam_monitor_t *mh)
{
  if (pthread_mutex_lock(&mh->mutex) != 0)
  { return(-1); }

  zhandle_t *zh          = mh->zh;
  pthread_mutex_t mutex  = mh->mutex;
  mh->zh                 = NULL;
  pthread_mutex_unlock(&mh->mutex);

  if (zh != NULL)
  { zookeeper_close(zh); }
  free(mh->znode);
  free(mh->endpoint);
  pthread_mutex_destroy(&mutex);
  free(mh);

  return(0);
}
