#include <stdio.h>
#include <stdlib.h>
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

shmem_result shmem_open(char *name)
{
    struct shmem_result result;
    result.id = name;

    mutex_lock();

    result.fd = shm_open(strjoin("/", name), flags, mode);

    mutex_release();

    if (result.fd > -1)
    {
        result.state = CREATED;
        ftruncate(result.fd, 0);
    }
    else
    {
        result.state = ERROR;
        switch (errno)
        {
        case EACCES:
            printf("Permission Exception.\n");
            break;
        case EEXIST:
            printf("Shared memory object specified by name already exists.\n");
            break;
        case EINVAL:
            printf("Invalid shared memory name passed.\n");
            break;
        case EMFILE:
            printf("The process already has the maximum number of files open.\n");
            break;
        case ENAMETOOLONG:
            printf("The length of name exceeds PATH_MAX.\n");
            break;
        case ENFILE:
            printf("The limit on the total number of files open on the system has been reached.\n");
            break;
        default:
            printf("Invalid exception occurred in shared memory creation.\n");
            break;
        }
    }

    return result;
}

shmem_result shmem_close(char *name)
{
    struct shmem_result result;

    mutex_lock();

    result.fd = shm_unlink(strjoin("/", name));

    mutex_release();

    result.id = name;
    result.state = CLOSED;

    return result;
}

bool shmem_exists(char *name)
{
    int fd = shm_open(strjoin("/", name), O_EXCL);

    return fd > -1;
}
/*
char *shmem_read(char *name)
{
    char *ret[0];
    if (shmem_exists(name))
    {
    }

    return ret;
}
*/
bool shmem_write(char *name, char *data)
{
    bool succeeded = false;
    if (shmem_exists(name))
    {
        mutex_lock();

        int data_size = strlen(data);
        struct stat buffer;

        int fd = shm_open(strjoin("/", name), flags, mode);
        fstat(fd, &buffer);

        printf("data size: %d \n", data_size);
        printf("shm size: %lld \n", buffer.st_size);
        
        void *ptr = mmap(NULL, data_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

        if (ptr == MAP_FAILED)
        {
            printf("Map failed. \n");
        }

        close(fd);

        memcpy(ptr, data, data_size);

        write(STDOUT_FILENO, ptr, data_size);       

        fstat(fd, &buffer);
        printf("\n");
        printf("shm size: %lld \n", buffer.st_size);
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

char *strjoin(char *strA, char *strB)
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

    shmem_write(result.id, "Hello World!");

    shmem_write(result.id, "Hello World!");

    struct shmem_result res = shmem_close(result.id);

    printf("id: %s, fd: %d, state: %d \n", res.id, res.fd, res.state);

    exit(0);
}