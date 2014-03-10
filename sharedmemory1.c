/* 
 * Mike Foley, mjfoley@gmail.com, 914-512-8705 */
 *
 * compiled on Ubuntu Linux
   gcc -pthread -g -o sharedmemory1 sharedmemory1.c
 *
 * this is the first program, it creates, initializes, and eventually destroys the shared memory region around updating it in cooperation with the second program.
 *
 * run with no arguments

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <signal.h> 

static volatile int signal_received=0;

static void
signal_handler(int sig)
	{
	signal_received=sig;
	return;
	}

int
main(int argc, char **argv)
	{
	union {
		char name[4];
		key_t key;
		} key = {'M','i','k','e'};
	typedef struct mine {
		pthread_mutex_t mutex;
		int array[5];
		} Mine;
	volatile Mine *region;
	int shmid;
	pthread_mutexattr_t mutex_attr;
	int just_created=0;
	time_t t;
	int i;	/* index value for loops */
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);
	setbuf(stderr,NULL);
	if ((shmid=shmget(key.key,0,0))==-1)	/* first try to attach to existing shared memory region */
		{
		if ((shmid=shmget(key.key,sizeof(region[0]),IPC_CREAT|0600))==-1)	/* first try to attach to existing shared memory region */
			return(fprintf(stderr,"%5u: Unable to create %ld byte region with key 0x%x: errno=%d %s\n",__LINE__,
				sizeof(region[0]),key.key,errno,strerror(errno)));
		just_created=1;
		}
	if ((region=shmat(shmid,NULL,0))==((Mine *)-1))
		{
		fprintf(stderr,"%5u: Unable to attach %ld byte region with key 0x%x and id %d: errno=%d %s \n",__LINE__,
			sizeof(region[0]),key.key,shmid,errno,strerror(errno));
		shmctl(shmid,IPC_RMID,0);
		return(__LINE__);
		}
	if (just_created)	/* if we just created the shared memory region, initialize the mutex and array */
		{
		if (pthread_mutexattr_init(&mutex_attr))
			{
			fprintf(stderr,"%5u: Unable to initialize mutex_attr for shared memory region id %d: errno=%d %s \n",__LINE__,
				shmid,errno,strerror(errno));
			shmdt(region);
			shmctl(shmid,IPC_RMID,0);
			return(__LINE__);
			}
		if (pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_ERRORCHECK_NP))
			{
			fprintf(stderr,"%5u: Unable to settype mutex_attr for shared memory region id %d: errno=%d %s \n",__LINE__,
				shmid,errno,strerror(errno));
			shmdt(region);
			shmctl(shmid,IPC_RMID,0);
			return(__LINE__);
			}
		if (pthread_mutexattr_setpshared(&mutex_attr,PTHREAD_PROCESS_SHARED))
			{
			fprintf(stderr,"%5u: Unable to settype mutex_attr for shared memory region id %d: errno=%d %s \n",__LINE__,
				shmid,errno,strerror(errno));
			shmdt(region);
			shmctl(shmid,IPC_RMID,0);
			return(__LINE__);
			}
		if (pthread_mutex_init(&(region->mutex),&mutex_attr))	/* initialize mutex process shared */
			{
			fprintf(stderr,"%5u: Unable to initialize pthread_mutex in shared memory region id %d: errno=%d %s \n",__LINE__,
				shmid,errno,strerror(errno));
			shmdt(region);
			shmctl(shmid,IPC_RMID,0);
			return(__LINE__);
			}
		for (i=0;i<(sizeof(region->array)/sizeof(region->array[0]));i++)
			region->array[i]=0;
		/*	printf("Creating and initializing shared memory region id %d\n",shmid); */
	}
	signal(SIGUSR1,signal_handler);
	signal(SIGUSR2,signal_handler);
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	for (;;)
		{
		switch (signal_received)
			{
			default: fprintf(stderr,"Unknown signal %d received, resetting\n",signal_received); /* fall through */
			 /* these signals DO NOT terminate the program */
			case SIGUSR1:
			case SIGUSR2:
				signal(signal_received,signal_handler);	/* reset signal handler */
				signal_received=0;
			case 0:
				break;
			/* these signals elegantly terminate the program */
			case SIGINT:
			case SIGTERM: 
				break;	/* leave signal_received non-zero to break loop */
			}
		if (signal_received) break;	/* if we got signaled to terminate the program, break out of for loop */
		if ((region->array[0]%2)==0)	/* this is the 1st process we will only increment even values to odd */
			{
			if (pthread_mutex_trylock(&region->mutex))	/* non zero return is failure to acquire lock */
			       {
				sleep(2);	/* resonable pause so screen is readable and processor not pinned */
			       }
			      else	/* trylock succeeds, this process owns region */
			       {
				time(&t);	
				printf("%s",ctime(&t));
				/* print array */
				for (i=0;i<(sizeof(region->array)/sizeof(region->array[0]));i++)
					printf(" %d",region->array[i]);
				/* change array */
				for (i=0;i<(sizeof(region->array)/sizeof(region->array[0]));i++)
					region->array[i]++;	/* increment values */
				printf(" changing to");
				/* print array */
				for (i=0;i<(sizeof(region->array)/sizeof(region->array[0]));i++)
					printf(" %d",region->array[i]);
				printf("\n");
				sleep(1);
				if (pthread_mutex_unlock(&region->mutex))
					{
					fprintf(stderr,"%5u: Unable to initialize pthread_mutex in shared memory region id %d: errno=%d %s \n",__LINE__,
						shmid,errno,strerror(errno));
					break;	/* unlock failure causes loop and then program to terminate */
					}
				}
			}
		       else	/* if this is 1st process we sleep when values are odd to allow 2nd process to make them even */
			{
			sleep(1);	/* array values are odd, it's not our turn */
			}
		}
	pthread_mutex_destroy(&region->mutex);
	shmdt(region);
	shmctl(shmid,IPC_RMID,0);
	return(0);
	}

