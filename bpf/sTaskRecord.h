/* Copyright (c) 2026, Alexey Bychko.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/* One thread of the target as the task iterator reports it. Shared by the BPF
   program and the sampler, so plain C with fixed-size fields. */

#pragma once

#define SSR_TASK_FRAMES 96
#define SSR_TASK_NAME 16

struct ssr_task_record {
  unsigned int tid;
  unsigned int state;    /* task->__state */
  long long syscall;     /* orig_ax of the saved user registers; -1 outside a call */
  unsigned long long arg0, arg1;
  unsigned long long ip, sp, bp;
  unsigned long long at_sp; /* the word at the stack pointer */
  unsigned int frames;      /* return addresses in ra[] */
  unsigned int complete;    /* the chain ended at a zero frame pointer */
  char name[SSR_TASK_NAME];
  unsigned long long ra[SSR_TASK_FRAMES];
};
