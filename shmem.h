#ifndef SHMEM_H
#define SHMEM_H

// to-be interface

typedef enum state state;
typedef struct shmem_result shmem_result;

shmem_result shmem_open(char *name);
shmem_result shmem_close(char *name);

bool shmem_exists(char *name);
bool shmem_write(char *name, char *data);
bool shmem_push(char *name, char *data);

char *shmem_read(char *name);

char *strjoin(char *strA, char *strB);
void mutex_lock();
void mutex_release();

#endif