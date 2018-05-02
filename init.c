#include <math.h>
#include <pthread.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <ucontext.h>

#include "dthreads.h"
#include "msg.h"

extern void *locks;
extern void *directory;
extern int lock_cnt;
extern int dir_count;
extern info_t infos[];

bool_t init_1_svc(node_t* nodes_info, void *result, struct svc_req *unused2)
{
  bool_t retval = 1;
  int i, j;

  printf("Initializing Server\n");

  n = nodes_info->num_nodes - 1;
  strcpy(me, nodes_info->nodes[0]);
  for (i = 1; i <= n; i++) {
    printf("Creating client with node %s\n", nodes_info->nodes[i]);
    strcpy(infos[i - 1].name, nodes_info->nodes[i]);
    infos[i - 1].server = clnt_create(infos[i - 1].name, DTHREADS, DTHREADS_V1, "tcp");

    for (j = 0; j < 10; j++)
      infos[i - 1].reply_deferred[j] = 0;
  }

  int lock_shmid = shmget(500, 40960, IPC_CREAT | 0666);
  locks = shmat(lock_shmid, (void *)0, 0);

  int dir_shmid = shmget(501, 40960, IPC_CREAT | 0666);
  directory = shmat(dir_shmid, (void *)0, 0);

  for (i = 0; i < 5; i++) {
    int j;
    dir_entry_t *dir = directory;
    dir_entry_t *entry = dir + (sizeof(dir_entry_t) * i);

    entry->my_presence = 0;
    for (j = 0; j < 10; j++) {
      entry->presence[j] = 0;
    }
    entry->st = INVALID;
    pthread_mutex_init(&entry->lock, NULL);
  }

  lock_cnt = 0;
  dir_count = 0;
  return retval;
}
