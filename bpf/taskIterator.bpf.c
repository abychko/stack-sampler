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

/* Task iterator: for every thread of the target, its state, the system call and
   user registers saved at kernel entry, and the frame-pointer chain from RBP,
   copied with bpf_copy_from_user_task. No thread is stopped, nothing is hooked.
   The approach follows xstack (0x.tools, Tanel Poder, GPL-2.0). */

#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>

#include "sTaskRecord.h"

char LICENSE[] SEC("license") = "GPL";

const volatile int target_tgid = 0;

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __uint(max_entries, 1);
  __type(key, __u32);
  __type(value, struct ssr_task_record);
} scratch SEC(".maps");

SEC("iter.s/task")
int walk(struct bpf_iter__task *ctx) {
  struct seq_file *seq = ctx->meta->seq;
  struct task_struct *task = ctx->task;
  if (!task || task->tgid != target_tgid) return 0;
  __u32 zero = 0;
  struct ssr_task_record *r = bpf_map_lookup_elem(&scratch, &zero);
  if (!r) return 0;
  r->tid = task->pid;
  r->state = task->__state;
  r->syscall = -1;
  r->arg0 = r->arg1 = r->ip = r->sp = r->bp = r->at_sp = 0;
  r->frames = 0;
  r->complete = 0;
  bpf_probe_read_kernel_str(r->name, sizeof(r->name), task->comm);
  if (r->state != 0) {
    struct pt_regs *regs = (struct pt_regs *)bpf_task_pt_regs(task);
    if (regs) {
      r->syscall = BPF_CORE_READ(regs, orig_ax);
      r->arg0 = BPF_CORE_READ(regs, di);
      r->arg1 = BPF_CORE_READ(regs, si);
      r->ip = BPF_CORE_READ(regs, ip);
      r->sp = BPF_CORE_READ(regs, sp);
      r->bp = BPF_CORE_READ(regs, bp);
      if (bpf_copy_from_user_task(&r->at_sp, sizeof(r->at_sp), (void *)r->sp, task, 0)) r->at_sp = 0;
      __u64 fp = r->bp;
      __u64 pair[2];
      for (int i = 0; i < SSR_TASK_FRAMES; i++) {
        if (fp == 0 || (fp & 7)) break;
        if (bpf_copy_from_user_task(pair, sizeof(pair), (void *)fp, task, 0)) break;
        r->ra[i] = pair[1];
        r->frames = i + 1;
        if (pair[0] == 0) {
          r->complete = 1;
          break;
        }
        if (pair[0] <= fp) break;
        fp = pair[0];
      }
    }
  }
  bpf_seq_write(seq, r, sizeof(*r));
  return 0;
}
