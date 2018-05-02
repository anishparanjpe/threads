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

#include "dthreads.h"
#include "msg.h"

int lock_cnt;
void *locks;

lock_t* get_lock(int lock_no)
{
  int i;
  lock_t *l = locks;

  for (i = 0; i < lock_cnt; i++) {
    if ((l + (sizeof(lock_t) * i))->lock_no == lock_no)
      return (l + (sizeof(lock_t) * i));
  }
  return NULL;
}

int get_lock_index(int lock_no)
{
  int i;
  lock_t *l = locks;

  for (i = 0; i < lock_cnt; i++) {
    if ((l + (sizeof(lock_t) * i))->lock_no == lock_no)
      return i;
  }
  return -1;
}


lock_t* create_lock(int lock_no) {
  lock_t *l = locks;
  lock_t *lock = l + (sizeof(lock_t) * lock_cnt);

  lock->lock_no = lock_no;
  lock->seq_no = 0;
  lock->highest_seq_no = 0;
  pthread_mutex_init(&lock->shared_vars, NULL);
  lock_cnt++;
  return lock;
}

