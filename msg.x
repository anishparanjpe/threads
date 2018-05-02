typedef char node_name[20];

struct node_t {
  node_name nodes[10];
  int num_nodes;
};

struct req_t {
  int lock_no;
  int seq_no;
  char node[20];
};

struct rep_t {
  int lock_no;
  char node[20];
};

struct local_t {
  char name[20];
  int size;
};

struct spoon_t {
  char name[20];
  char node_name[20];
};

typedef char buf[1024];

program DTHREADS {
  version DTHREADS_V1 {
    void INIT(node_t) = 1;

    void ACQUIRE_LOCK(int) = 2;
    void RELEASE_LOCK(int) = 3;

    void REQUEST(req_t) = 4;
    void REPLY(rep_t) = 5;

    int MALLOC(local_t) = 6;
    void FREE(string) = 7;

    int PREAD(string) = 8;
    int PWRITE(string) = 9;

    void NMALLOC(spoon_t) = 10;
    void NFREE(spoon_t) = 11;
    buf NREAD(string) = 12;
    void NINV(spoon_t) = 13;
  } = 1;
} = 0x2ffffff;

