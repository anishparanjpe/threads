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

int dir_count;
void *directory;

dir_entry_t* get_entry(char *name)
{
  int i;
  dir_entry_t *dir = directory;

  for (i = 0; i < dir_count; i++) {
    if (!strcmp((dir + (sizeof(dir_entry_t) * i))->name, name))
      return dir + (sizeof(dir_entry_t) * i);
  }
  return NULL;
}

dir_entry_t* create_entry(char *name) {
  dir_entry_t *dir = directory;
  dir_entry_t *entry = dir + (sizeof(dir_entry_t) * dir_count);

  strcpy(entry->name, name);

  dir_count++;
  return entry;
}


int get_owner(dir_entry_t *entry)
{
  int i;

  for (i = 0; i < 10; i++)
    if (entry->presence[i])
      return i;
  return -1;
}

