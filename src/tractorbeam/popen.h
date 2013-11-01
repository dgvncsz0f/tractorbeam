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

#ifndef __tractorbeam_popen_h__
#define __tractorbeam_popen_h__

typedef struct tractorbeam_popen_t tractorbeam_popen_t;

/*! Creates a new process
 *
 * \param stdout The handle that allows you to consume the stdout from the process;
 *
 * \param prog The path of the image to execute (This should be an
 *             absolute path);
 *
 * \param argv Arguments to pass to the image being run. The argv[0]
 *             should be set to the absolute path of the program being
 *             run. This must not be null and the last member of this
 *             array must be NULL.
 *
 * \param envv Environment variables. This may be NULL in which case
 *             the current environment is used.
 *
 * \return The handle to this process or NULL, is there was any error.
 */
tractorbeam_popen_t *tractorbeam_popen_init(const char *prog, char * const *argv, char * const *envv);

/*! Returns the filehandle you can use to consume the process output.
 */
int tractorbeam_popen_fd(tractorbeam_popen_t *);

/*! Terminates the current process.
 *
 * \param timeout How much time before send a SIGKILL to the process
 *                (in case it still running);
 * 
 * \return The exit code of the process;
 */
int tractorbeam_popen_term(tractorbeam_popen_t *, int timeout);

#endif
