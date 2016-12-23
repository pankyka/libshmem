#ifndef SHMEM_H
#define SHMEM_H

#include <stdbool.h>

typedef enum state state;
typedef struct shmem_result shmem_result;

shmem_result shmem_open(const char *name);
shmem_result shmem_close(const char *name);
bool shmem_exists(const char *name);
bool shmem_write(const char *name, const char *data);
bool shmem_push(const char *name, const char *data);
char *shmem_read(const char *name);

char *get_name(const char *name);
char *strjoin(const char *strA, const char *strB);
void mutex_lock();
void mutex_release();

#endif