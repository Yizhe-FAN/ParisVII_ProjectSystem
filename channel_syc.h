#include <stdbool.h> /* bool */
#include <errno.h>   /* errno */
#include <stdlib.h>  /* NULL */
#include <stdio.h>   /* printf */
#include <unistd.h>  /* sleep */
#include <string.h>  /* memcpy */
#include <pthread.h> /* pthread_xxx */
#include <sys/mman.h> /* mmap, PROT_READ, PROT_WRITE */
#include <sys/stat.h>
#include <fcntl.h>    /* O_RDWR, O_CREAT */
#include <semaphore.h>

struct Node
{
	void *data;
	struct Node *next;
};

struct liste{
  struct Node* pHead;
  struct Node* pTail;
  int size;
};

struct channel{
	void *data;
	int id;
	int front;
	int rear;
	int eltSize;
	int totalSize;
	int storedDataNum;
	int functionFlag;
	bool isClosed;
	pthread_mutex_t lock;
	pthread_cond_t condEmpty, condFull, waitRead, waitWrite;
	struct liste* liste;
};

/* flags */
#define CHANNEL_PROCESS_SHARED 1

/*
 * Utilities
 * */
int increase_qindex(int current, int total);

/* channel_create will return NULL upon fail creation.
 * When parameter size is given 0, return NULL with errno ENOSYS
 * When parameter flags is set, but we did not implement "canaux globaux(4.3)", return NULL with errno ENOSYS
 */
struct channel *channel_create(int eltsize, int size, int flags);

/* channel_destroy will only disable the function without synchronization.  */
void channel_destroy(struct channel *channel);

/*
 * Return -1 when channel is already closed with errno EPIPE.
 * Return  1 and store the value of the data (not the pointer) when the list is less than the max capacity.
 * When the list reaches the max capacity, blocks the thread and waits until there is space.
 * */
int channel_send(struct channel *channel, const void *data);

/* channel_close should stop any elements being added but still allow being accessed.
 * Return  1 when success.
 * Return  0 when channel is already closed.
 * Return -1 when error */
int channel_close(struct channel *channel);

/*
 * Return 1 and filled the first-in element in data when the list is not empty.
 * Return 0 when the channel is closed.
 * When the list is empty and the channel is not closed, wait until it is filled.
 *  */
int channel_recv(struct channel *channel, void *data);

