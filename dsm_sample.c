#include <pthread.h>
#include <rpc/rpc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>

extern void psu_init_lock_mgr(char**, int);
extern void* psu_dsm_malloc(char*,size_t);
extern void psu_dsm_free(char*);

int main(int argc, char **argv)
{
  psu_init_lock_mgr(argv + 1, argc - 1);

  char ch;
  void* data;
  while (1) {
    printf("Enter operation: (m/f/r/w): ");
    scanf("%c", &ch);

    char name[20], val;
    int offset;
    switch(ch) {
      case 'm':
        printf("Enter memory name: ");
        scanf("%s", name);
        data = psu_dsm_malloc(name, 1024);
        printf("Address: %p\n", data);
        break;
      case 'f':
        printf("Enter memory name: ");
        scanf("%s", name);
        psu_dsm_free(name);
        break;
      case 'r':
        printf("Enter memory name and offset: ");
        scanf("%s %d", name, &offset);
        printf("%s = %c\n", name, *((char *)data + offset));
        break;
      case 'w':
        printf("Enter memory name offset and new value: ");
        scanf("%s %d %c", name, &offset, &val);
        *((char *)data + offset) = val;
        printf("%s = %c\n", name, *((char *)data + offset));
        break;
    }
    getchar();
  }
  return 0;
}
