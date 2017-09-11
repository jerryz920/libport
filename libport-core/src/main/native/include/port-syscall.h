/*
   The FreeBSD Copyright

   Copyright 1992-2017 The FreeBSD Project. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.  Redistributions in binary
   form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials
   provided with the distribution.  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND
   CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   The views and conclusions contained in the software and documentation are
   those of the authors and should not be interpreted as representing official
   policies, either expressed or implied, of the FreeBSD Project.

   Special system call for client side port management.
   Author: Yan Zhai

*/
#ifndef _PORT_SYSCALL_H
#define _PORT_SYSCALL_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__x86_64__)
#define GET_PROC_LOCAL_PORTS	      326	
#define SET_PROC_LOCAL_PORTS	      327	
#define GET_PROC_RESERVED_PORTS	      328	
#define ADD_PROC_RESERVED_PORTS	      329	
#define DELETE_PROC_RESERVED_PORTS    330	
#define CLEAR_PROC_RESERVED_PORTS     331	

#elif defined (__i386__)
#define GET_PROC_LOCAL_PORTS	      377	
#define SET_PROC_LOCAL_PORTS	      378	
#define GET_PROC_RESERVED_PORTS	      379	
#define ADD_PROC_RESERVED_PORTS	      380	
#define DELETE_PROC_RESERVED_PORTS    381	
#define CLEAR_PROC_RESERVED_PORTS     382	
#else
#error "not supported architecture!"
#endif

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>


static inline int get_local_ports(int *lo, int *hi) {
  return syscall(GET_PROC_LOCAL_PORTS, 0, lo, hi);
}

static inline int set_local_ports(int lo, int hi) {
  return syscall(SET_PROC_LOCAL_PORTS, 0, lo, hi);
}

static inline int get_child_ports(pid_t pid, int *lo, int *hi) {
  return syscall(GET_PROC_LOCAL_PORTS, pid, lo, hi);
}

static inline int set_child_ports(pid_t pid, int lo, int hi) {
  return syscall(SET_PROC_LOCAL_PORTS, pid, lo, hi);
}

static inline int get_reserved_ports(int *lo, int *hi) {
  return syscall(GET_PROC_RESERVED_PORTS, 0, lo, hi);
}

static inline int add_reserved_ports(int lo, int hi) {
  return syscall(ADD_PROC_RESERVED_PORTS, 0, lo, hi);
}

static inline int del_reserved_ports(int lo, int hi) {
  return syscall(DELETE_PROC_RESERVED_PORTS, 0, lo, hi);
}

static inline int clear_reserved_ports() {
  return syscall(CLEAR_PROC_RESERVED_PORTS, 0);
}



#ifdef __cplusplus
}
#endif


#endif
