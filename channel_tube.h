#include <stdbool.h> /* bool */
#include <errno.h>   /* errno */
#include <stdlib.h>  /* NULL */
#include <stdio.h>   /* printf */
#include <unistd.h>  /* sleep, read, write, close */
#include <sys/types.h> /*read, write  */
#include <sys/uio.h>   /*read, write  */
#include <pthread.h> /* pthread_xxx */

struct channel_tube
{
	int tubeId;
	int tubeIndex[2];
	int id;
	int eltSize;
	bool volatile isClosed;
};

/* flags */
#define CHANNEL_PROCESS_SHARED 1

/* channel_create will return NULL upon fail creation.
 * When parameter size is given 0, return NULL with errno ENOSYS
 * When parameter flags is set, but we did not implement "canaux globaux(4.3)", return NULL with errno ENOSYS
 */
struct channel_tube *channel_create(int eltsize, int size, int flags);

/* channel_destroy will only disable the function without synchronization.  */
void channel_destroy(struct channel_tube *channel);

/*
 * Return -1 when channel is already closed with errno EPIPE.
 * Return  1 and store the value of the data (not the pointer) when the list is less than the max capacity.
 * When the list reaches the max capacity, blocks the thread and waits until there is space.
 * */
int channel_send(struct channel_tube *channel, const void *data);

/* channel_close should stop any elements being added but still allow being accessed.
 * Return  1 when success.
 * Return  0 when channel is already closed.
 * Return -1 when error */
int channel_close(struct channel_tube *channel);

/*
 * Return 1 and filled the first-in element in data when the list is not empty.
 * Return 0 when the channel is closed.
 * When the list is empty and the channel is not closed, wait until it is filled.
 *  */
int channel_recv(struct channel_tube *channel, void *data);

