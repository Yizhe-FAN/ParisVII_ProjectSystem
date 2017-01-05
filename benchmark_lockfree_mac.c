#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "channel_lockfree_mac.h"

/**
 * This structure is used to pass 2 arguments to our thread (patient/doctor) methods
 */
struct thread_args
{
	struct channel* chl;
    int id;
};

/**
 * Patients produce questions to ask doctors
 */
void* patient(void* args)
{
	int count = 1000;
	while(count>0)
	{
		int rand = random()%10000;
		struct thread_args thargs = *(struct thread_args*)args;
		char* msg = malloc(256);
		sprintf(msg, "Question%d from Patient_%d", rand, thargs.id);
		int rc = channel_send(thargs.chl, msg);
		if(rc<=0)
			perror("channel_send");
		else
		{
			//printf("'%s' sent\n", msg);
		}
		count--;
	}
	return NULL;
}

/**
 * Doctors consume the questions in the mailbox from the patients
 */
void* doctor(void* args)
{
	int count = 1000;
	while(count>0)
	{
		struct thread_args thargs = *(struct thread_args*)args;
		char* msg = malloc(256);
		int rc = channel_recv(thargs.chl, msg);
		if(rc<=0)
			perror("channel_send");
		else
		{
			//printf("Doctor_%d answered: '%s'\n", thargs.id, msg);
		}
		count--;
	}
	return NULL;
}

/**
 * The scenario of this example :
 *
 *   Several patients ask questions to a mailbox of a clinic which the doctors will read these questions and response.
 *
 * We will create 6 threads as doctors and 12 threads as patients while we will use channel as the mailbox to communicate between patients and doctors.
 * The mailbox is created once with 3 maximal messages inside where the length of each message is 256 bytes.
 *
 */
int main(int argc, char **argv)
{
	int numThreads = 18;
	int numMaxData = 3;
	int channelType = 0;
	const char *usage = "./benchmark.exe [-n numthreads] [-m maxdata] [-t impletype]";

	while(1)
	{
		int opt = getopt(argc, argv, "n:m:t:");
		if(opt < 0)
			break;

		switch(opt)
		{
		case 'm':
			numMaxData = atoi(optarg);
			break;
		case 'n':
			numThreads = atoi(optarg);
			break;
		case 't':
			channelType = atoi(optarg);
			break;
		default:
			fprintf(stderr, "%s\n", usage);
			exit(1);
		}
	}

	struct timeval t0, t1;
	gettimeofday(&t0, NULL);
	srandom(time(NULL));
	int nbOfDoctors = numThreads/2;
	int nbOfPatients = numThreads/2;
	int nbOfMsgBox = numMaxData;
	printf("Total send/receive: %d with message box size %d  \n", nbOfPatients*1000, nbOfMsgBox );

	pthread_t threadDoctors[nbOfDoctors]; //Consumer threads
	pthread_t threadPatients[nbOfPatients]; //Producer threads

	struct channel* mailBox = channel_create(256, nbOfMsgBox, channelType); //Intermediate communicator

	for(int i = 0; i < nbOfDoctors; i++)
	{
		struct thread_args* args = malloc(sizeof(struct thread_args));
		args->chl = mailBox;
		args->id = i;
		int rc = pthread_create(&threadDoctors[i], NULL, &doctor, args);
		if(rc != 0)
		{
			errno = rc;
			perror("pthread_create");
			exit(1);
		}
	}

    for(int i = 0; i < nbOfPatients; i++)
    {
		struct thread_args* args = malloc(sizeof(struct thread_args));
		args->chl = mailBox;
		args->id = i;
        int rc = pthread_create(&threadPatients[i], NULL, &patient, args);
        if(rc != 0)
        {
            errno = rc;
            perror("pthread_create");
            exit(1);
        }
    }

	for(int i = 0; i < nbOfDoctors; i++)
	{
		pthread_join(threadDoctors[i], NULL);
	}

    for(int i = 0; i < nbOfPatients; i++)
    {
    	pthread_join(threadPatients[i], NULL);
	}

    channel_close(mailBox);
    channel_destroy(mailBox);

    gettimeofday(&t1, NULL);
    printf("Run in %.2lfs\n",((double)t1.tv_sec - t0.tv_sec) + ((double)t1.tv_usec - t0.tv_usec) / 1.0E6);
}
