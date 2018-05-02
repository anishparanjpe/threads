#ifndef DTHREAD_H
#define DTHREAD_H

#include <rpc/rpc.h>

int n;
char me[20];

enum state { MODIFIED, SHARED, INVALID };

typedef struct info_s {
  char name[20];
  CLIENT *server;
  int reply_deferred[10];
} info_t;

typedef struct lock_s {
  int lock_no;
  int seq_no;
  int highest_seq_no;
  int outstanding_reply_cnt;
  int requesting_cs;
  pthread_mutex_t shared_vars;
} lock_t;

typedef struct dir_entry {
  char name[20];
  int shmid;
  int my_presence;
  int presence[10];
  enum state st;
  pthread_mutex_t lock;
} dir_entry_t;

typedef struct seg_s {
  char name[20];
  enum state st;
  int size;
  void *data;
} seg_t;

#endif
