/*
 * example.c
 *
 *  Created on: 16 avr. 2016
 *      Author: Quincy
 *
 *  If you found errors or have any question about the code,
 *  Please feel free to email quincy.tw(at)gmail.com. Thanks!
 */
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "channel.h"

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
	int count = 0;
	struct thread_args thargs = *(struct thread_args*)args;
	while(1)
	{
		char* msg = malloc(256);
		sprintf(msg, "Question_%d from Patient_%d", count, thargs.id);
		int rc = channel_send(thargs.chl, msg);
		if(rc<=0)
			perror("channel_send");
		else
		{
			printf("'%s' sent\n", msg);
			count++;
			sleep(3);
		}
	}
}

/**
 * Doctors consume the questions in the mailbox from the patients
 */
void* doctor(void* args)
{
	struct thread_args thargs = *(struct thread_args*)args;
	while(1)
	{
		char* msg = malloc(256);
		int rc = channel_recv(thargs.chl, msg);
		if(rc<=0)
			perror("channel_send");
		else
		{
			printf("Doctor_%d answered: '%s'\n", thargs.id, msg);
		}
	}
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
	int nbOfDoctors = 6;
	int nbOfPatients = 12;
	int nbOfMsgBox = 3;

	pthread_t threadDoctors[nbOfDoctors]; //Consumer threads
	pthread_t threadPatients[nbOfPatients]; //Producer threads

	struct channel* mailBox = channel_create(256, nbOfMsgBox, 0); //Intermediate communicator

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
}
