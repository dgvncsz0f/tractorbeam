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

#ifndef __tractorbeam_monitor_h__
#define __tractorbeam_monitor_h__

#include <stdlib.h>

typedef struct tractorbeam_monitor_t tractorbeam_monitor_t;

typedef enum
{
  ITEM,
  DONE,
  FAIL
} tb_snapshot_events;

/*! tractorbeam_monitor_snapshot callback.
 *
 * This function gets called for every node found in along the
 * requested path.
 *
 * \param event The event that has ocurred. DONE or FAIL means the
 *              function has terminated wither successfuly or not
 *              respectively;
 * 
 * \param ppath The (absolute) path of the parent node. This is NULL
 *              when event is DONE or FAIL;
 *
 * \param name The name of the current node. ppath + / + name creates
 *             the absolute path of the curret node;
 *
 * \param contents The contents of the node (might be NULL);
 *
 * param contsize The size of the data pointer (0 if data is NULL);
 * 
 * \return 0: continue;
 *         else: aborts the function;
 */
typedef int (*tb_snapshot_fn)(tb_snapshot_events event, const char *ppath, const char *name, const void *contents, size_t contsize, void *data);

/*! Initialize the monitor.
 *
 * This reports the application state on the given zookeeper
 * cluster. This function does not do much as it only establishes the
 * connection. You should use tractorbeam_monitor_update to report the
 * state.
 *
 * \param zk_endpoint Zookeeper cluster to use;
 *
 * \param znode The path of the ephemeral node to create (all the
 *              parents nodes must exist);
 * 
 * \param timeout_in_ms Zookeeper session timeout in milliseconds;
 *
 * \param data What to write on zookeeper;
 *
 * \return The monitor handle or NULL if there was any error;
 */
tractorbeam_monitor_t *tractorbeam_monitor_init(const char *zk_endpoint, const char *znode, int timeout_in_ms);

/*! Writes data onto the znode.
 *
 * This function shall create or set the znode on zookeeper with the
 * given data.
 *
 * \param data The data you want to write. May be NULL, in which case
 *             the data gets deleted (the znode continues, though);
 *
 * \param datasize The size of the data param. Not used if data is
 *                 NULL;
 *
 * \return 0: success;
 *
 * \return 1: you must retry the operation (an stale ephemeral node has been deleted, for instance);
 *
 * \return -2: could not create/update the znode;
 *
 * \return -1: error;
 */
int tractorbeam_monitor_update(tractorbeam_monitor_t *, const void *data, size_t datasize);

/*! Walks a given zookeeper tree.
 */
int tractorbeam_monitor_snapshot(tractorbeam_monitor_t *, const char *path, tb_snapshot_fn callback, void *data);

/*! Deletes the znode from zookeeper;
 *
 *  \return 0: success;
 *
 *  \return -1: error;
 */
int tractorbeam_monitor_delete(tractorbeam_monitor_t *);

/*! Free all resources used by this monitor.
 *
 * \return 0: success;
 * 
 * \return -1: failure;
 */
int tractorbeam_monitor_term(tractorbeam_monitor_t *);

#endif
