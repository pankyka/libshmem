#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "shmem.h"

int flags = O_RDWR | O_CREAT;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

enum state
{
    ERROR = -1,
    CLOSED = 0,
    CREATED = 1
};

struct shmem_result
{
    state state;
    char *id;
    int fd;
};

shmem_result shmem_open(const char *name)
{
    struct shmem_result result;
    result.id = (char *)name;

    mutex_lock();

    result.fd = shm_open(get_name(name), flags, mode);

    mutex_release();

    if (result.fd > -1)
    {
        result.state = CREATED;
        ftruncate(result.fd, 0);
    }
    else
    {
        result.state = ERROR;
        //perror(errno);
    }

    return result;
}

shmem_result shmem_close(const char *name)
{
    struct shmem_result result;

    mutex_lock();

    result.fd = shm_unlink(get_name(name));

    mutex_release();

    result.id = (char *)name;
    result.state = CLOSED;

    return result;
}

bool shmem_exists(const char *name)
{
    int fd = shm_open(get_name(name), O_EXCL, 0);

    return fd > -1;
}

char *shmem_read(const char *name)
{
    char *ret;
    struct stat buffer;
    if (shmem_exists(name))
    {        
        mutex_lock();
        
        int fd = shm_open(get_name(name), flags, mode);
        fstat(fd, &buffer);
        ret = (char *)malloc(buffer.st_size);
        ret = mmap(NULL, buffer.st_size, PROT_READ, MAP_SHARED, fd, 0);

        mutex_release();

        if (ret == MAP_FAILED)
        {
            printf("Map failed. \n");            
        }
    }

    return ret;
}

bool shmem_write(const char *name, const char *data)
{
    bool succeeded = false;
    if (shmem_exists(name))
    {
        mutex_lock();

        int data_size = strlen(data);
        struct stat buffer;

        int fd = shm_open(get_name(name), flags, mode);
        fstat(fd, &buffer);
        ftruncate(fd, data_size + buffer.st_size);   

        data_size += buffer.st_size;

        void *ptr = mmap(NULL, data_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

        if (ptr == MAP_FAILED)
        {
            printf("Map failed. \n");            
        }

        close(fd);

        memcpy(ptr, strcat(ptr, data), data_size);

        write(STDOUT_FILENO, ptr, data_size);       

        fstat(fd, &buffer);
        printf("\n");
        printf("shm size: %llu \n", (unsigned long long)buffer.st_size);
        printf("-------------------\n");

        mutex_release();
        succeeded = true;
    }

    return succeeded;
}

void mutex_lock()
{
    pthread_mutex_lock(&mutex);
}

void mutex_release()
{
    pthread_mutex_unlock(&mutex);
}

char *get_name(const char *name) 
{
    return strjoin("/", name);
}

char *strjoin(const char *strA, const char *strB)
{
    size_t lenA = strlen(strA);
    size_t lenB = strlen(strB);
    char *ret = (char *)malloc(lenA + lenB + 1);
    strcpy(ret, strA);
    strcat(ret, strB);
    return ret;
}

int main(void)
{
    const char *named = "pupu";

    shmem_close(named);

    struct shmem_result result = shmem_open(named);

    printf("id: %s, fd: %d, state: %d \n", result.id, result.fd, result.state);

    bool is_exists = shmem_exists(result.id);

    printf("exists: %d \n", is_exists);

    shmem_write(result.id, "Hello");

    shmem_write(result.id, "Hello");

    shmem_write(result.id, "Hello");

    char *ret = shmem_read(named);

    write(STDOUT_FILENO, ret, strlen(ret));       

    struct shmem_result res = shmem_close(result.id);

    printf("\nid: %s, fd: %d, state: %d \n", res.id, res.fd, res.state);

    exit(0);
}