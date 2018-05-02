#include <pthread.h>
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

extern seg_t* create_seg(char*, enum state, char*);

CLIENT *client;
void *directory;

void* psu_dsm_malloc(char *name, size_t size)
{
  int id;

  local_t *local = malloc(sizeof(local_t));
  strcpy(local->name, name);
  local->size = size;

  malloc_1(local, &id, client);

  char *data = shmat(id, (void *)0, 0);
  create_seg(name, MODIFIED, data);

  return (void *) data;
}

void psu_dsm_free(char *name)
{
  free_1(&name, NULL, client);
}
