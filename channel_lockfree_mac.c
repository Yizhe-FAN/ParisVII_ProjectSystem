#include "channel_lockfree.h"

/**
     int64_t
     OSAtomicIncrement64Barrier(volatile int64_t *theValue);

     int64_t
     OSAtomicDecrement64(volatile int64_t *theValue);

     bool
     OSAtomicCompareAndSwapPtrBarrier(void* oldValue, void* newValue, void* volatile *theValue);

     bool
     OSAtomicTestAndSetBarrier(uint32_t n, volatile void *theAddress);

     bool
     OSAtomicTestAndClearBarrier(uint32_t n, volatile void *theAddress);

 */
static int nCreatCount = 0;
struct channel *channel_create(int eltsize, int size, int flags)
{
	int numOfCpu = sysconf(_SC_NPROCESSORS_ONLN);
	long pageSize = sysconf(_SC_PAGESIZE);
	int arch = 64;
	if ((size_t)-1 < 0xffffffffUL)
		arch = 32;

	struct channel* chl;
	if(flags==CHANNEL_PROCESS_SHARED)
	{
		if(size==0)
		{
			errno = ENOSYS;
			return NULL;
		}
		int fdSharedChl = -1;
		int fdSharedData = -1;
		void* addrChl = mmap(NULL, sizeof(struct channel), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, fdSharedChl , 0);
		if (addrChl == MAP_FAILED)
			return NULL;
		else
			chl = addrChl;

		void* addrData = mmap(NULL, eltsize*size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, fdSharedData , 0);
		if (addrData == MAP_FAILED)
			return NULL;
		else
			chl->data = addrData;

		chl->functionFlag = CHANNEL_PROCESS_SHARED;
		chl->totalSize = size;
		pthread_mutexattr_t mattr;
  		pthread_mutexattr_init(&mattr);
  		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  		pthread_mutex_init(&(chl->lock), &mattr);

  		pthread_condattr_t cattr;
  		pthread_condattr_init(&cattr);
  		pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

  		pthread_cond_init(&(chl->waitingSend),&cattr );
  		pthread_cond_init(&(chl->waitingReceive),&cattr );
  		printf("Shared channel %d created with system: %d CPU, page size %ld, arch %d-bits\n", nCreatCount, numOfCpu, pageSize, arch);
	}
	else if(flags==CHANNEL_TUBE_IMP)
	{
		chl = malloc(sizeof(struct channel));
		chl->tubeId = pipe(chl->tubeIndex);
		chl->data = NULL;//Not used
		chl->totalSize = 0;//Not used
		chl->functionFlag = CHANNEL_TUBE_IMP;
		printf("Pipe channel %d created with system: %d CPU, page size %ld, arch %d-bits\n", nCreatCount, numOfCpu, pageSize, arch);
	}
	else if(flags==CHANNEL_LOCK_FREE_IMP)
	{
		//TODO
		chl = malloc(sizeof(struct channel));
		chl->data = NULL;
		chl->totalSize = 0;//Not used
		chl->functionFlag = CHANNEL_LOCK_FREE_IMP;
		chl->volatileTurn = 0; //0 = send ; 1 = recv
		chl->volatileSetData = 0;
		printf("Lock-free channel %d created with system: %d CPU, page size %ld, arch %d-bits\n", nCreatCount, numOfCpu, pageSize, arch);
	}
	else
	{
		chl = malloc(sizeof(struct channel));
		if(flags==CHANNEL_PROCESS_SYNCHRON || size==0) //When size is 0, we initial the variables differently
		{
			chl->data = NULL;
			chl->totalSize = 0;
			chl->functionFlag = CHANNEL_PROCESS_SYNCHRON;
			printf("Synchronized channel %d created with system: %d CPU, page size %ld, arch %d-bits\n", nCreatCount, numOfCpu, pageSize, arch);
		}
		else
		{
			chl->data = malloc( eltsize*size );
			chl->totalSize = size;
			chl->functionFlag = flags; //Default = 0
			printf("Non-synchronized channel %d created with system: %d CPU, page size %ld, arch %d-bits\n", nCreatCount, numOfCpu, pageSize, arch);
		}
		pthread_mutex_init(&(chl->lock), NULL);
		pthread_cond_init(&(chl->waitingSend), NULL);
		pthread_cond_init(&(chl->waitingReceive), NULL);
	}
	chl->id = nCreatCount;
	chl->eltSize = eltsize;
	chl->front = 0;
	chl->rear = 0;
	chl->isClosed = false;
	chl->storedDataNum = 0;
	chl->arch = arch;
	nCreatCount++;
	return chl;
}

/* When using this fonction, i think we should not forget to do 'free()' */
void channel_destroy(struct channel *channel)
{
	pthread_mutex_destroy(&(channel->lock));
	pthread_cond_destroy(&(channel->waitingSend));
	pthread_cond_destroy(&(channel->waitingReceive));
	if(channel->functionFlag == CHANNEL_PROCESS_SHARED)
	{
		munmap( channel->data, (channel->eltSize)*(channel->totalSize));
		munmap( channel, sizeof(struct channel) );
	}
	else
	{
		free(channel->data);
		free(channel);
	}
}

int increase_qindex(int current, int total)
{
	int inc = (current+1)%total;
	return inc;
}

int channel_send(struct channel *channel, void *data)
{
	if (channel->isClosed)
	{
		errno = EPIPE;
		return -1;
	}
	int rt = 0;
	if(channel->functionFlag == CHANNEL_TUBE_IMP)
	{
		rt = write(channel->tubeIndex[1], data, channel->eltSize);
		if(rt<0)
		{
			errno = EPIPE;
			rt = -1;
		}
		else
		{
			//printf("[>>]Data sent to chl_%d @ tube_%d = %d\n",channel->id, channel->tubeId, *((int*)data));
			rt = 1;
		}
	}
	else if(channel->functionFlag == CHANNEL_LOCK_FREE_IMP)
	{
		//TODO
		printf("channel_send %d called\n", channel->id);
		if(channel->volatileTurn!=0)
		{
			while(!OSAtomicCompareAndSwapIntBarrier(1, 0, &channel->volatileTurn))
			{
				printf("channel_send %d waits to enter CS. Turn=%d\n", channel->id, channel->volatileTurn);
				sleep(1);
			}
		}

		if(channel->volatileSetData!=2)
		{
			while(!OSAtomicCompareAndSwapIntBarrier(1, 2, &channel->volatileSetData))
			{
				OSAtomicCompareAndSwapIntBarrier(0, 1, &channel->volatileTurn);
				printf("channel_send %d waits to copy data. Turn=%d SetData=%d\n", channel->id, channel->volatileTurn, channel->volatileSetData);
				sleep(1);
			}
		}

		memcpy(channel->data, data, channel->eltSize);
		printf("channel_send %d copied. Turn=%d SetData=%d\n", channel->id, channel->volatileTurn, channel->volatileSetData);
		OSAtomicCompareAndSwapIntBarrier(2, 0, &channel->volatileSetData);

		while(!OSAtomicCompareAndSwapIntBarrier(0, 1, &channel->volatileTurn))
		{
			printf("channel_send %d waits to exit CS. Turn=%d\n", channel->id, channel->volatileTurn);
			sleep(1);
		}
		rt = 1;

	}
	else
	{
		//In both normal and synchron, we need lock to control the communication
		pthread_mutex_lock(&(channel->lock));
		if(channel->functionFlag == CHANNEL_PROCESS_SYNCHRON)
		{
			while(channel->totalSize == 0)
			{
				pthread_cond_wait(&channel->waitingReceive, &channel->lock);
			}
			memcpy(channel->data, data, channel->eltSize);
			channel->data = NULL;
			channel->totalSize--;
			//printf("Finished sent the data to the reader %p\n", channel->data);
			pthread_cond_broadcast(&channel->waitingSend);
			rt = 1;
		}
		else
		{
			while (channel->storedDataNum == channel->totalSize)
			{
				pthread_cond_wait(&channel->waitingReceive, &channel->lock);
			}

			int index = (channel->rear)*(channel->eltSize);
			memcpy(channel->data+index, data, channel->eltSize);
			channel->rear = increase_qindex(channel->rear, channel->totalSize);
			channel->storedDataNum++;
			//printf("[>>]Data sent to chl_%d: %d/%d ;F->R: %d->%d\n",channel->id, channel->storedDataNum, channel->totalSize, channel->front, channel->rear);
			pthread_cond_broadcast(&(channel->waitingSend));
			rt = 1;
		}
		pthread_mutex_unlock(&(channel->lock));
	}
	return rt;
}

int channel_close(struct channel *channel)
{
	int rt = -1;
	if(channel->functionFlag == CHANNEL_TUBE_IMP)
	{
		if(channel->isClosed)
		{
			rt = 0;
		}
		else
		{
			int rc = close(channel->tubeIndex[0]);
			if(rc<0)
				perror("Unable to close file desc 0");
			rc = close(channel->tubeIndex[1]);
			if(rc<0)
				perror("Unable to close file desc 1");

			channel->isClosed = true;
			rt = 1;
		}
	}
	else
	{
		pthread_mutex_lock(&(channel->lock));
		if(channel->isClosed)
		{
			rt = 0;
		}
		else
		{
			channel->isClosed = true;
			rt = 1;
		}
		pthread_mutex_unlock(&(channel->lock));
	}
	return rt;
}

int channel_recv(struct channel *channel, void *data)
{
	if(channel->isClosed)
	{
		return 0;
	}

	int rt = -1;
	if(channel->functionFlag == CHANNEL_TUBE_IMP)
	{
		int rd = read(channel->tubeIndex[0], data, channel->eltSize);
		if(rd>=0) //If read successful, set flag to 1
		{
			//printf("[<<]Data recv from chl_%d @ tube_%d = %d\n",channel->id, channel->tubeId, *((int*)data));
			rt = 1;
		}
	}
	else if(channel->functionFlag == CHANNEL_LOCK_FREE_IMP)
	{
		printf("channel_recv %d called\n", channel->id);
		//TODO when volatileStoredDataNum=0 -> no one in recv nor send | 1 -> someone in recv | -> -1 -> someone in send
		if(channel->volatileTurn!=1)
		{
			while(!OSAtomicCompareAndSwapIntBarrier(0, 1, &channel->volatileTurn))
			{
				printf("channel_recv %d waits to enter CS. Turn=%d\n", channel->id, channel->volatileTurn);
				sleep(1);
			}
		}

		while(!OSAtomicCompareAndSwapIntBarrier(0, 1, &channel->volatileSetData))
		{
			printf("channel_recv %d waits to set %p <- %p. Turn=%d\n", channel->id, channel->data, data, channel->volatileTurn);
			OSAtomicCompareAndSwapIntBarrier(1, 0, &channel->volatileTurn);
			sleep(1);
		}
		channel->data = data;
		printf("channel_recv %d set %p <- %p. Turn=%d\n", channel->id, channel->data, data, channel->volatileTurn);
		while(!OSAtomicCompareAndSwapIntBarrier(0, 0, &channel->volatileSetData))
		{
			printf("channel_recv %d waits message to be copied. Turn=%d\n", channel->id, channel->volatileTurn);
			OSAtomicCompareAndSwapIntBarrier(1, 0, &channel->volatileTurn);
			sleep(1);
		}
		printf("channel_recv %d exits. Turn=%d\n", channel->id, channel->volatileTurn);

	}
	else
	{
		pthread_mutex_lock(&(channel->lock));
		if(channel->functionFlag == CHANNEL_PROCESS_SYNCHRON)
		{
			while(channel->totalSize == 1) //Wait existing data to be copied by send
			{
				pthread_cond_wait(&channel->waitingSend, &channel->lock);
			}
			//Set address to be sent
			channel->data = data;
			channel->totalSize++;
			//printf("Received data and user's data pointers are %p <- %p\n", channel->data, data);
			pthread_cond_broadcast(&channel->waitingReceive); //Notify address has been set
			while( channel->data != NULL) //Wait my data to been copied by send
			{
				pthread_cond_wait(&channel->waitingSend, &channel->lock);
			}
			rt = 1;
		}
		else
		{
			//Block the process when there is no message
			while(channel->storedDataNum==0)
			{
				pthread_cond_wait(&(channel->waitingSend), &(channel->lock));
			}
			int index = (channel->front)*(channel->eltSize);
			memcpy(data, channel->data+index, channel->eltSize);
			channel->front = increase_qindex(channel->front, channel->totalSize);
			channel->storedDataNum--;
			//printf("[<<]Data recv from chl_%d: %d/%d ;F->R: %d->%d\n",channel->id, channel->storedDataNum, channel->totalSize, channel->front, channel->rear);
			pthread_cond_broadcast(&(channel->waitingReceive));
			rt = 1;
		}
		pthread_mutex_unlock(&(channel->lock));
	}
	return rt;
}
