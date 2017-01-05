#include "channel_tube.h"

static int nCreatCount = 0;

struct channel_tube *channel_create(int eltsize, int size, int flags)
{
	if(size==0 || flags==CHANNEL_PROCESS_SHARED)
	{
		errno = ENOSYS;
		return NULL;
	}

	struct channel_tube* chl = malloc(sizeof(struct channel_tube));
	chl->id = nCreatCount;
	chl->tubeId = pipe(chl->tubeIndex);
	chl->eltSize = eltsize;
	chl->isClosed = false;
	nCreatCount++;
	return chl;
}

/* When using this fonction, i think we should not forget to do 'free()' */
void channel_destroy(struct channel_tube *channel)
{

}

int increase_qindex(int current, int total)
{
	int inc = (current+1)%total;
	return inc;
}

int channel_send(struct channel_tube *channel, const void *data)
{
	int rt = 0;
	if (channel->isClosed)
	{
		errno = EPIPE;
		rt = -1;
	}
	else
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
	return rt;
}

int channel_close(struct channel_tube *channel)
{
	int rt = -1;
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
	return rt;
}

int channel_recv(struct channel_tube *channel, void *data)
{
	int rt = -1;
	if(channel->isClosed)
	{
		rt = 0;
	}
	else
	{
		int rd = read(channel->tubeIndex[0], data, channel->eltSize);
		if(rd>=0) //If read successful, set flag to 1
		{
			//printf("[<<]Data recv from chl_%d @ tube_%d = %d\n",channel->id, channel->tubeId, *((int*)data));
			rt = 1;
		}
	}
	return rt;
}
