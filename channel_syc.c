#include "channel_syc.h"

static int nCreatCount = 0;

struct liste* create_list(){
  struct liste* ls = malloc(sizeof(struct liste));
  if(ls==NULL) return NULL;
  ls->pHead = NULL;
  ls->pTail = NULL;
  ls->size = 0;
  return ls;
}

void add_addr(struct liste* l, void* addr){
  struct Node* nd = malloc(sizeof(struct Node));
  nd->data=addr;
  nd->next=NULL;
  if(l->size==0){
    l->pHead=nd;
    l->pTail=nd;
  }
  else{
    struct Node* last = l->pTail;
    last->next=nd;
    l->pTail=nd;
  }
  (l->size)++;
}

void* pop_addr(struct liste* l){
  if(l->size==0) return NULL;
  struct Node* nd = l->pHead; 
  l->pHead=nd->next;
  (l->size)--;
  if(l->size==0){
    l->pHead=NULL;
    l->pTail=NULL;
  } 
  void* addr = nd->data;
  free(nd);
  return addr;
}

struct channel *channel_create(int eltsize, int size, int flags)
{
	struct channel* chl;
	if(size == 0)
	{

		chl = malloc(sizeof(struct channel));
		chl->data = NULL;
		pthread_mutex_init(&(chl->lock), NULL);
		pthread_cond_init(&(chl->waitRead),NULL);
		pthread_cond_init(&(chl->waitWrite),NULL);

		//errno = ENOSY//S;
		//return NULL;
	}

	
	else if(flags==CHANNEL_PROCESS_SHARED)
	{
		int fdSharedChl = -1;
		int fdSharedData = -1;
		void* addrChl = mmap(NULL, sizeof(struct channel), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, fdSharedChl , 0);
		if (addrChl == MAP_FAILED)
			return NULL;
		// else
			chl = addrChl;

		void* addrData = mmap(NULL, eltsize*size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, fdSharedData , 0);
		if (addrData == MAP_FAILED)
			return NULL;
		else
			chl->data = addrData;

		pthread_mutexattr_t mattr;
  		pthread_mutexattr_init(&mattr);
  		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  		pthread_mutex_init(&(chl->lock), &mattr);

  		pthread_condattr_t cattr;
  		pthread_condattr_init(&cattr);
  		pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

  		pthread_cond_init(&(chl->condEmpty),&cattr );
  		pthread_cond_init(&(chl->condFull),&cattr );
	}
	else
	{
		//Initial channel with eltsize and size
		chl = malloc(sizeof(struct channel));
		chl->data = malloc( eltsize*size );
		pthread_mutex_init(&(chl->lock), NULL);
		pthread_cond_init(&(chl->condEmpty), NULL);
		pthread_cond_init(&(chl->condFull), NULL);
	}

	chl->id = nCreatCount;
	chl->eltSize = eltsize;
	chl->totalSize = size;
	chl->front = 0;
	chl->rear = 0;
	chl->isClosed = false;
	chl->storedDataNum = 0;
	chl->functionFlag = flags;

	chl->liste = create_list();

	nCreatCount++;
	return chl;
}

/* When using this fonction, i think we should not forget to do 'free()' */
void channel_destroy(struct channel *channel)
{
	pthread_mutex_destroy(&(channel->lock));
	
	if(channel->totalSize == 0 ){
		pthread_cond_destroy(&(channel->waitWrite));
		pthread_cond_destroy(&(channel->waitRead));	
		free(channel->liste);
		free(channel);
	}
	else if(channel->functionFlag == CHANNEL_PROCESS_SHARED)
	{

		pthread_cond_destroy(&(channel->condEmpty));
		pthread_cond_destroy(&(channel->condFull));
		munmap( channel->data, (channel->eltSize)*(channel->totalSize));
		munmap( channel, sizeof(struct channel) );
	}
	else
	{
		pthread_cond_destroy(&(channel->condEmpty));
		pthread_cond_destroy(&(channel->condFull));
		free(channel->data);
		free(channel);
		free(channel->liste);
	}
}

int increase_qindex(int current, int total)
{
	int inc = (current+1)%total;
	return inc;
}

int channel_send(struct channel *channel, const void *data)
{
	int rt = 0;
	pthread_mutex_lock(&(channel->lock));
	if (channel->isClosed)
	{
		errno = EPIPE;
		rt = -1;
		pthread_mutex_unlock(&(channel->lock));
	}
	else 
	{
		if(channel->totalSize == 0)
		{		
				
			while(channel->liste->size == 0)
			{
				//printf(">>>waiting for the reader>>>\n");
				pthread_cond_wait(&channel->waitRead, &channel->lock);
			}			
			//printf("There is a reader, ready to sent the data to %p\n", channel->liste->pHead);
						

			memcpy(channel->liste->pHead->data, data, channel->eltSize);
			//printf("finished sent the data to the reader %p\n", channel->liste->pHead->data);
			
			channel->liste->pHead->data = NULL;
			//printf(">>>delete the address of the reader <%p> in the canal>>>\n", channel->liste->pHead->data);
			channel->liste->pHead = channel->liste->pHead->next;
			channel->liste->size--;

			//printf("Informed the reader to delete\n");
			pthread_mutex_unlock(&(channel->lock));
			pthread_cond_broadcast(&channel->waitWrite);
			
			
			
		}
		else{
			while (channel->liste->size == channel->totalSize)
			{
				//printf("Channel is full. Waiting an available spot...\n");
				pthread_cond_wait(&channel->condFull, &channel->lock);
			}			
			void *espace = malloc(channel->eltSize);
			add_addr(channel->liste, espace);
			memcpy(channel->liste->pTail->data, (void*)data, channel->eltSize);

			//printf("[>>]Data sent to chl_%d: %d/%d ;F->R: %d->%d\n",channel->id, channel->storedDataNum, channel->totalSize, channel->front, channel->rear);
			pthread_mutex_unlock(&(channel->lock));
			pthread_cond_broadcast(&(channel->condEmpty));
		}		
		rt = 1;
	}	
	return rt;
}

int channel_close(struct channel *channel)
{
	int rt = -1;
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
	return rt;
}

int channel_recv(struct channel *channel, void *data)
{
	int rt = -1;
	pthread_mutex_lock(&(channel->lock));
	if(channel->isClosed && channel->storedDataNum == 0)
	{
		rt = 0;
		pthread_mutex_unlock(&(channel->lock));
	}
	else
	{
		if(channel->totalSize == 0)
		{
			//printf(">>>Reader is present and preparing>>>\n");

			//printf("the port is %p\n", data);
			add_addr(channel->liste, data);

			pthread_cond_broadcast(&channel->waitRead);	
			//printf("port is open in %p of %p et informed the writer\n", channel->liste->pTail->data, channel->liste->pTail);

			struct Node *node = channel->liste->pTail;
			//printf("data is saved in %p\n", node->data);

			while( node->data != NULL)
			{
				//printf(">>>waiting for writer >>>\n");
				pthread_cond_wait(&channel->waitWrite, &channel->lock);
			}
			//printf("receive the data and now data is %p of the %p\n", node->data, node);
			free(node); 

			//printf(">>>deleted the reader>>>\n");			
			pthread_mutex_unlock(&(channel->lock));
			//pthread_cond_broadcast(&channel->waitRead);
							
		}
		else{
			while(channel->liste->size == 0)
			{
				//Block the process when there is no message
				//printf("Channel is empty. Waiting to receive new data in channel...\n");
				pthread_cond_wait(&(channel->condEmpty), &(channel->lock));
			}
			void* espace = pop_addr(channel->liste);
			memcpy(data, espace, channel->eltSize);
			//printf("[<<]Data recv from chl_%d: %d/%d ;F->R: %d->%d\n",channel->id, channel->storedDataNum, channel->totalSize, channel->front, channel->rear);	
			pthread_mutex_unlock(&(channel->lock));
			pthread_cond_broadcast(&(channel->condFull));
		}	
		rt = 1;
	}
	return rt;
}
