#include <pthread.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void psu_init_lock_mgr(char**, int);
extern void psu_acquire_lock(int);
extern void psu_release_lock(int);

void *handler(void *ptr)
{
  int lock_number = *(int *) ptr;
  printf("Acquiring lock %d\n", lock_number);
  psu_acquire_lock(lock_number);
  printf("Lock %d acquired\n", lock_number);
  sleep(10);
  printf("Releasing lock %d\n", lock_number);
  psu_release_lock(lock_number);
  printf("Lock %d released\n", lock_number);
  return NULL;
}

int main(int argc, char **argv)
{
  psu_init_lock_mgr(argv + 1, argc - 1);

  int lock_number;
  while (1) {
    printf("Enter lock number: ");
    scanf("%d", &lock_number);
    getchar();
    pthread_t thread;
    pthread_create(&thread, NULL, handler, &lock_number);
  }
  return 1;
}
