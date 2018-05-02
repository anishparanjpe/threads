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

#define MAX(a,b) (((a)>(b))?(a):(b))

extern lock_t* get_lock(int);
extern int get_lock_index(int);
extern lock_t* create_lock(int);

extern dir_entry_t* get_entry(char *);
extern dir_entry_t* create_entry(char *);
extern int get_owner(dir_entry_t *);

req_t* make_request(int lock_no, int seq_no, char* node)
{
  req_t *req = malloc(sizeof(req_t));

  req->lock_no = lock_no;
  req->seq_no = seq_no;
  strcpy(req->node, node);
  return req;
}

rep_t* make_reply(int lock_no, char* node)
{
  rep_t *rep = malloc(sizeof(rep_t));

  rep->lock_no = lock_no;
  strcpy(rep->node, node);
  return rep;
}

spoon_t* make_spoon(char *name, char *node_name)
{
  spoon_t *spoon = malloc(sizeof(spoon_t));
  strcpy(spoon->name, name);
  strcpy(spoon->node_name, node_name);
  return spoon;
}

info_t infos[10];

info_t* get_server(char *name)
{
  int i;

  for (i = 0; i < n; i++) {
    if (!strcmp(infos[i].name, name)) {
      return &infos[i];
    }
  }
  return NULL;
}

int get_server_index(char* name)
{
  int i;

  for (i = 0; i < 10; i++) {
    if (infos[i].server == NULL) continue;
    if (!strcmp(infos[i].name, name))
      return i;
  }
  return -1;
}

typedef struct temp_s {
  int lock_no;
  int i;
} temp_t;

bool_t acquire_lock_1_svc(int *lock_number, void *result, struct svc_req *unused)
{
  bool_t retval = 1;
  int i;

  printf("Acquiring lock %d\n", *lock_number);

  lock_t* lock = get_lock(*lock_number);
  if (lock == NULL) {
    lock = create_lock(*lock_number);
  }

  pthread_mutex_lock(&lock->shared_vars);
  lock->requesting_cs = 1;
  lock->seq_no = lock->highest_seq_no + 1;
  pthread_mutex_unlock(&lock->shared_vars);

  lock->outstanding_reply_cnt = n;

  req_t *req = make_request(lock->lock_no, lock->seq_no, me);
  for (i = 0; i < 10; i++) {
    if (infos[i].server != NULL) {
      printf("Sending request to %s for lock %d with seq no: %d\n", infos[i].name, *lock_number, lock->seq_no);
      request_1(req, NULL, infos[i].server);
    }
  }

  return retval;
}

bool_t release_lock_1_svc(int *lock_number, void *result, struct svc_req *unused)
{
  bool_t retval = 1;
  lock_t* lock = get_lock(*lock_number);
  int i;

  lock->requesting_cs = 0;

  int idx = get_lock_index(*lock_number);
  for (i = 0; i < 10; i++) {
    if (infos[i].server == NULL) continue;
    if (infos[i].reply_deferred[idx]) {
      infos[i].reply_deferred[idx] = 0;
      reply_1(make_reply(lock->lock_no, me), NULL, infos[i].server);
    }
  }

  return retval;
}

void *request_handler(void *ptr)
{
  req_t *req = (req_t *) ptr;
  lock_t* lock = get_lock(req->lock_no);

  if (lock == NULL) {
    lock = create_lock(req->lock_no);
  }
  int idx = get_lock_index(req->lock_no);
  int defer_it = 0;

  pthread_mutex_lock(&lock->shared_vars);
  lock->highest_seq_no = MAX(lock->highest_seq_no, req->seq_no);

  info_t *info = get_server(req->node);
  if (lock->requesting_cs
      && ((req->seq_no > lock->seq_no)
          || (req->seq_no == lock->seq_no && strcmp(me, req->node)))) {
    printf("Deferring reply for lock %d from %s as (Req's seq no) %d > (Lock's seq no) %d\n", req->lock_no,
        req->node, req->seq_no, lock->seq_no);
    defer_it = 1;
  }
  pthread_mutex_unlock(&lock->shared_vars);

  if (defer_it) info->reply_deferred[idx] = 1;
  else reply_1(make_reply(req->lock_no, me), NULL, info->server);

  return NULL;
}

bool_t request_1_svc(req_t *req, void *result, struct svc_req *unused)
{
  bool_t retval = 1;

  printf("Received request from %s for lock %d with seq no: %d\n", req->node, req->lock_no, req->seq_no);
  req_t *tmp = make_request(req->lock_no, req->seq_no, req->node);
  pthread_t thread;
  pthread_create(&thread, NULL, request_handler, tmp);
  return retval;
}

bool_t reply_1_svc(rep_t *rep, void *result, struct svc_req *unused)
{
  bool_t retval = 1;
  lock_t *lock = get_lock(rep->lock_no);

  printf("Received reply from %s for lock %d\n", rep->node, rep->lock_no);
  lock->outstanding_reply_cnt -= 1;
  return retval;
}

bool_t malloc_1_svc(local_t *local, int *id, struct svc_req *unused2)
{
  bool_t retval = 1;

  int i = rand() % 100;
  int shmid = shmget(i, 1024, IPC_CREAT | 0666);

  char *data, *tmp = malloc(local->size);
  char *name = malloc(20);

  strcpy(name, local->name);

  dir_entry_t *entry = get_entry(local->name);
  if (entry == NULL)
    entry = create_entry(local->name);
  pthread_mutex_lock(&entry->lock);

  entry->shmid = shmid;
  data = shmat(shmid, (void *)0, 0);
  if (entry->st == MODIFIED
      || (entry->st == INVALID && (get_owner(entry) != -1))) {
    nread_1(&name, tmp, infos[get_owner(entry)].server);
    entry->st = SHARED;
    memcpy(data, tmp, 1024);
  }
  else {
    entry->my_presence = 1;

    entry->st = MODIFIED;

    int j;
    for (j = 0; j < 10; j++) {
      if (infos[j].server == NULL) continue;
      nmalloc_1(make_spoon(local->name, me), NULL, infos[j].server);
    }
  }
  *id = shmid;
  pthread_mutex_unlock(&entry->lock);
  return retval;
}

bool_t free_1_svc(char **name, void *unused, struct svc_req *unused2)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(*name);
  if (entry == NULL)
    entry = create_entry(*name);
  pthread_mutex_lock(&entry->lock);

  entry->my_presence = 0;
  entry->st = INVALID;

  int j;
  for (j = 0; j < 10; j++) {
    if (infos[j].server == NULL) continue;
    nmalloc_1(make_spoon(*name, me), NULL, infos[j].server);
  }

  pthread_mutex_unlock(&entry->lock);

  return retval;
}

bool_t pread_1_svc(char **name, int *i, struct svc_req *unused)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(*name);
  if (entry == NULL)
    entry = create_entry(*name);

  char *data = shmat(entry->shmid, (void *)0, 0);
  pthread_mutex_lock(&entry->lock);

  if (entry->st == INVALID) {
    nread_1(name, data, infos[get_owner(entry)].server);
    entry->st = SHARED;
  }
  *i = 0;
  pthread_mutex_unlock(&entry->lock);
  return retval;
}

bool_t pwrite_1_svc(char **name, int *i, struct svc_req *unused)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(*name);
  if (entry == NULL)
    entry = create_entry(*name);

  char *data = shmat(entry->shmid, (void *)0, 0);
  pthread_mutex_lock(&entry->lock);

  if (entry->st == INVALID)
    nread_1(name, data, infos[get_owner(entry)].server);

  if (entry->st == SHARED || entry->st == INVALID) {
    int j;

    for (j = 0; j < 10; j++) {
      if (infos[j].server == NULL) continue;
      ninv_1(make_spoon(*name, me), NULL, infos[j].server);
    }
  }

  entry->st = MODIFIED;

  pthread_mutex_unlock(&entry->lock);
  return retval;
}

bool_t nmalloc_1_svc(spoon_t *spoon, void *unused, struct svc_req *unused2)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(spoon->name);
  if (entry == NULL)
    entry = create_entry(spoon->name);

  pthread_mutex_lock(&entry->lock);

  printf("Initial copy of %s is with %s\n", spoon->name, spoon->node_name);
  int idx = get_server_index(spoon->node_name);
  entry->st = MODIFIED;
  entry->presence[idx] = 1;

  pthread_mutex_unlock(&entry->lock);
  return retval;
}

bool_t nfree_1_svc(spoon_t *spoon, void *unused, struct svc_req *unused2)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(spoon->name);
  if (entry == NULL)
    entry = create_entry(spoon->name);

  pthread_mutex_lock(&entry->lock);

  int idx = get_server_index(spoon->node_name);
  entry->presence[idx] = 0;

  pthread_mutex_unlock(&entry->lock);
  return retval;
}

bool_t nread_1_svc(char **name, char *buf, struct svc_req *unused2)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(*name);
  if (entry == NULL)
    entry = create_entry(*name);

  char *data = shmat(entry->shmid, (void *)0, 0);
  printf("Received read request for %s\n", *name);
  if (entry->st == MODIFIED)
    entry->st = SHARED;
  memcpy(buf, data, 1024);

  return retval;
}

bool_t ninv_1_svc(spoon_t *spoon, void *unused2, struct svc_req *unused3)
{
  bool_t retval = 1;

  dir_entry_t *entry = get_entry(spoon->name);
  if (entry == NULL)
    entry = create_entry(spoon->name);

  pthread_mutex_lock(&entry->lock);
  printf("Received invalidate request for %s from %s\n", spoon->name, spoon->node_name);
  int i;
  int idx = get_server_index(spoon->node_name);

  entry->st = INVALID;
  entry->my_presence = 0;
  for (i = 0; i < 10; i++)
    entry->presence[i] = 0;
  entry->presence[idx] = 1;

  pthread_mutex_unlock(&entry->lock);
  return retval;
}

bool_t migrate_1_svc(char *buf, void *unused2, struct svc_req *unused3)
{
  bool_t retval = 1;

  printf("Thread migrated successfully\n");

  return retval;
}

int dthreads_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
  xdr_free (xdr_result, result);
  return 1;
}
