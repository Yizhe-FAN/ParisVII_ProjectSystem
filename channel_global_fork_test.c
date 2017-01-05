#include "channel.h"
#include <sys/types.h>
#include <sys/wait.h>

#define QSIZE 3
#define NUMPROCE 7
#define LONGEUR 30

/**
 * In this example, we created two channels --
 * One is used to send message from father to son.
 * The other is used to send message from son to father.
 * At the end the father will check the sum of the series of numbers sent and received.
 */
int main (int argc, char** argv){
	
	/*channel 1 est pour distribuer les valeurs*/
	struct channel* chanFatherToSon = channel_create(sizeof(int),QSIZE,1);
	/*channel 2 est pour récupérer les resultat*/
	struct channel* chanSonToFather = channel_create(sizeof(int),QSIZE,1);

	int i;
	pid_t pid;

	/*create fils processus*/
	for(i = 0 ; i < NUMPROCE; i++)
	{
		pid = fork();
		if(pid==0) break;
	}

	if(pid < 0)
	{
		perror("fork failed\n");
	}
	else if(pid == 0) //Son
	{
		int data=0;
		while(1)
		{
			channel_recv(chanFatherToSon, &data);
			printf("Child proccess %d received value %d from to channel Father-To-Son\n", getpid(), data);
			if(data == -1)
			{
				channel_send(chanFatherToSon, &data);
				printf("Child proccess %d notify exit with value %d to to channel Father-To-Son\n", getpid(), data);
				//si la valeur qu'il recois est -1, ca veut dire le pere est fini de distribuer la valeur.
				//car il y a plus de travail a faire . le fils dois finir son travail.
				//Donc il donc cette valeur -1 dans le chan1 pour informer l'autre fils et quitter.
				break;
			}

			channel_send(chanSonToFather, &data);
			printf("Child proccess %d re-sent value %d to channel Son-To-Father\n", getpid(), data);
			//le fils passe la valeur qu'il recois dans le chan2
		}
		exit(0);
	}

	int tab[LONGEUR];
	int sommeEnvoye = 0;
	int sommeRecu = 0;
	for(i = 0; i< LONGEUR; i++)
	{
		tab[i] = i;
		sommeEnvoye += tab[i];
	}

	//initialiser le tableau et faire 'sum' moi-meme.
	for(i = 0; i< LONGEUR; i++)
	{
		channel_send(chanFatherToSon, &tab[i]);
		printf("Father proccess %d sent value %d to channel Father-To-Son\n", getpid(), tab[i]);
		//distribuer les valeurs dans chan1 aux fils.
		//printf("j'ai envoyé dans le channel %d et mon pid est %d\n", (*data), getpid());
		int data_calcul = 0;
		channel_recv(chanSonToFather, &data_calcul);
		printf("Father proccess %d received value %d from channel Son-To-Father\n", getpid(), data_calcul);
		//récupérer les resultat que les fils envoient et faire 'sum'
		sommeRecu += data_calcul;
	}

	//envoyer valeur -1 pour dire aux fils, le travail est fini.
	int fini = -1;
	channel_send(chanFatherToSon, &fini);
	printf("Father proccess %d notify calculation finished with value %d to channel Father-To-Son\n", getpid(), fini);
				
	while(wait(NULL)>0);
	printf("Sum sent from father is %d and sum received is %d\n", sommeEnvoye, sommeRecu);

	channel_destroy(chanSonToFather);
	channel_destroy(chanFatherToSon);
	return 0;
	
}
