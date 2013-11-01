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

#ifndef __tractorbeam_exec_h__
#define __tractorbeam_exec_h__

#define TRACTORBEAM_BUFFER_SIZE 4096

/*! Runs a program and collect its output.
 *
 * This function shall block until either the program terminates or
 * the timer times out.
 *
 * \param prg The program to run;
 *
 * \param timeout_in_sec The maximum allowed time (in seconds) for
 *                       this script to run;
 * 
 * \param ecode The exit code of the program;
 *
 * \param out The variable that will get the program output;
 *
 * \param outsz The size of the out param;
 *
 * \return >0 The program has successfully terminated (number of bytes read);
 *
 * \return -1 The program has failed to start or there was an error
 *            reading the output;
 *
 * \return -2 The program has timed out;
 *
 * \return -3 Buffer was too small;
 */
int tractorbeam_exec(const char *prg, char * const *argv, int timeout_in_sec, int *ecode, void *out, size_t outsz);

#endif
