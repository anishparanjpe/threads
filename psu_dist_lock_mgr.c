#include <rpc/rpc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "dthreads.h"
#include "msg.h"

CLIENT *client;
void *locks;
void *directory;

lock_t* get_lock(int lock_no)
{
  int i;
  lock_t *l = locks;

  for (i = 0; i < 10; i++) {
    if ((l + (sizeof(lock_t) * i))->lock_no == lock_no)
      return (l + (sizeof(lock_t) * i));
  }
  return NULL;
}

int seg_cnt = 0;
seg_t segs[10];

seg_t* create_seg(char *name, enum state st, char *data)
{
  seg_t *seg = &segs[seg_cnt];

  strcpy(seg->name, name);
  seg->st = st;
  seg->size = 1024;
  seg->data = data;
  seg_cnt++;
  return seg;
}

seg_t* get_seg(char *name)
{
  int i;

  for (i = 0; i < seg_cnt; i++)
    if (!strcmp(segs[i].name, name))
      return &segs[i];
  return NULL;
}

void *query_state(void *ptr)
{
  int i;
  dir_entry_t *dir = directory;

  while (1) {
    for (i = 0; i < 5; i++) {
      int st = (dir + (sizeof(dir_entry_t) * i))->st;

      if (st == segs[i].st) continue;
      segs[i].st = st;

      switch (st) {
        case SHARED:
          mprotect(segs[i].data, 1024, PROT_READ); break;
        case INVALID:
          mprotect(segs[i].data, 1024, PROT_NONE); break;
        case MODIFIED:
          mprotect(segs[i].data, 1024, PROT_READ | PROT_WRITE); break;
      }
    }
  }
  return NULL;
}

void segv_handler(int sig, siginfo_t *si, void *unused)
{
  int i, res;
  char *name;

  void *addr = si->si_addr;

  for (i = 0; i < seg_cnt; i++) {
    if (addr >= segs[i].data && addr < segs[i].data + segs[i].size) {
      name = malloc(20);
      strcpy(name, segs[i].name);
      switch (segs[i].st) {
        case SHARED:
          pwrite_1(&name, &res, client);
          mprotect(segs[i].data, segs[i].size, PROT_READ | PROT_WRITE); break;
        case INVALID:
          name = malloc(20);
          strcpy(name, segs[i].name);
          pread_1(&name, &res, client);
          mprotect(segs[i].data, segs[i].size, PROT_READ);
          break;
        case MODIFIED: break;
      }
    }
  }
}


void psu_init_lock_mgr(char **nodes, int num_nodes)
{
  int i;

  client = clnt_create("localhost", DTHREADS, DTHREADS_V1, "tcp");

  node_t *nodes_info = malloc(sizeof(node_t));

  for (i = 0; i < num_nodes; i++) {
    strcpy(nodes_info->nodes[i], nodes[i]);
  }
  nodes_info->num_nodes = num_nodes;
  init_1(nodes_info, NULL, client);

  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = segv_handler;
  sigaction(SIGSEGV, &sa, NULL);

  int lock_shmid = shmget(500, 40960, IPC_CREAT | 0666);
  locks = shmat(lock_shmid, (void *)0, 0);

  int dir_shmid = shmget(501, 40960, IPC_CREAT | 0666);
  directory = shmat(dir_shmid, (void *)0, 0);

  pthread_t thread;
  pthread_create(&thread, NULL, query_state, NULL);
}

void psu_acquire_lock(int lock_number)
{
  acquire_lock_1(&lock_number, NULL, client);

  while (1) {
    lock_t *lock = get_lock(lock_number);
    if (lock->outstanding_reply_cnt == 0) break;
  }
}

void psu_release_lock(int lock_number)
{
  release_lock_1(&lock_number, NULL, client);
}
