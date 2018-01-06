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
#define ALLOC_PROC_LOCAL_PORTS     332

#elif defined (__i386__)
#define GET_PROC_LOCAL_PORTS	      377	
#define SET_PROC_LOCAL_PORTS	      378	
#define GET_PROC_RESERVED_PORTS	      379	
#define ADD_PROC_RESERVED_PORTS	      380	
#define DELETE_PROC_RESERVED_PORTS    381	
#define CLEAR_PROC_RESERVED_PORTS     382	
#define ALLOC_PROC_LOCAL_PORTS     383
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

static inline int alloc_child_ports(pid_t ppid, pid_t pid, int n) {
  return syscall(ALLOC_PROC_LOCAL_PORTS, ppid, pid, n);
}



#ifdef __cplusplus
}
#endif

#include <fstream>

class SyscallProxy {
  public:
    SyscallProxy() {}
    virtual ~SyscallProxy() {}
    virtual int set_child_ports(pid_t pid, int lo, int hi) = 0;
    virtual int get_local_ports(int* lo, int *hi) = 0;
    virtual int add_reserved_ports(int lo, int hi) = 0;
    virtual int del_reserved_ports(int lo, int hi) = 0;
    virtual int clear_reserved_ports() = 0;
    virtual int alloc_child_ports(pid_t ppid, pid_t pid, int n) = 0;
};


class SyscallProxyImpl: public SyscallProxy {
  public:
    SyscallProxyImpl() {
      /// system usually reserves 32768-60000 ports to process as client.
      // Then if we update the port manager to 0-65535 then sometimes the set_local_port
      // does not work as expected. Instead, we read out the available local ports and
      // match it against get_child_ports
      int lo, hi;
      ::get_local_ports(&lo, &hi);

      int syslo, syshi;
      get_system_local_ports(&syslo, &syshi);

      if (lo < syslo || hi > syshi) {
        if (lo < syslo) lo = syslo;
        if (hi > syshi) hi = syshi;
        ::set_local_ports(lo, hi);
      }
    }

    int set_child_ports(pid_t pid, int lo, int hi) override {
      return ::set_child_ports(pid, lo, hi);
    }
    int get_local_ports(int* lo, int *hi) override {
      return ::get_local_ports(lo, hi);
    }
    int add_reserved_ports(int lo, int hi) override {
      return ::add_reserved_ports(lo, hi-1);
    }
    int del_reserved_ports(int lo, int hi) override {
      return ::del_reserved_ports(lo, hi-1);
    }
    int clear_reserved_ports() override {
      return ::clear_reserved_ports();
    }
    int alloc_child_ports(pid_t ppid, pid_t pid, int n) override {
      return ::alloc_child_ports(ppid, pid, n);
    }

    void get_system_local_ports(int *lo, int *hi) {
      std::ifstream f("/proc/sys/net/ipv4/ip_local_port_range");
      f >> *lo >> *hi;
    }
};

#endif
