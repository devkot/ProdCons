#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#define SHMSIZ 256

union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

int main(int argc, char * argv[]){
	int prodSem, consSem, semControl, ShMidP, ShMidC, N, i, lines, K, try;
	char *shmemP, *shmemC, line[SHMSIZ];
	FILE *fp1;
	pid_t pid;
	struct sembuf prodBuffer, consBuffer;
	union semun arg;


	if (argc != 3){
		printf("Wrong number of arguments\n"); 
		return -1;
	}

	N = atoi(argv[1]); //number of child processes
	K = atoi(argv[2]);//number of tries

	fp1 = fopen("blabla", "r"); //open file
	if(fp1==NULL){
		printf("cant open file\n");
		exit(1);
	}

	lines=0; //initalize lines
	while(fgets(line,SHMSIZ,fp1)!=NULL){  //count file lines
		lines++;
	}
	printf("LINES: %d\n",lines); 

	//file pointer to first char
	fseek(fp1,0,SEEK_SET);

    /*------------------Setting semaphores----------------------------*/
	if((prodSem = semget((key_t)1111,1,IPC_CREAT|0600)) <0){ //initialize prod sem
		perror("semget");
		exit(1);
	}
	arg.val=1; //setting value to 1
	if((semControl = semctl(prodSem, 0, SETVAL, arg)) < 0){
		perror("semctl");
		exit(1);
	}


	if((consSem = semget((key_t)2222, 1, IPC_CREAT|0600)) < 0){ //initialize cons sem
		perror("semget");
		exit(1);
	}
	arg.val=0; //set value to 0
	if((semControl = semctl(consSem,0, SETVAL, arg)) < 0){
		perror("semctl");
		exit(1);
	}


	/*-----------------Shared mem(producer)--------------------------------------*/
	if((ShMidP = shmget((key_t)9876, SHMSIZ, 0600|IPC_CREAT)) < 0){ //create shared mem
		perror("shmget");
		exit(1);
	}
	if((shmemP = shmat(ShMidP, (char*)0, 0)) == (char *) -1){  //attach shared mem locally
		perror("shmat");
		exit(1);
	}
	//Wipe producers shared mem
	int temp_iterator;
	for(temp_iterator=0;temp_iterator<SHMSIZ;temp_iterator++) 
		shmemP[temp_iterator]=0;
	
	printf("Attached producer mem\n");

	/*----------------Shared mem(consumer)---------------------------------------*/
	if((ShMidC = shmget((key_t)1234, SHMSIZ, 0600|IPC_CREAT)) < 0){
		perror("shmget");
		exit(1);
	}
	if((shmemC = shmat(ShMidC, (char*)0, 0)) == (char *) -1){
		perror("shmat");
		exit(1);
	}

	//Wipe the consumers shared mem
	for(temp_iterator=0;temp_iterator<SHMSIZ;temp_iterator++) 
		shmemC[temp_iterator]=0;
	

	printf("Attached consumer mem\n");

	/*----------------------------------------------------------*/
	long curtime=time(NULL);//seed for srand
	int j=0,random_line=0, g=0, pid_match=0,comp,flag=1;
	char msg[SHMSIZ],to_cap[SHMSIZ],get_back[SHMSIZ],buffer[SHMSIZ], buffer2[SHMSIZ];
	srand((unsigned int)curtime);
	

	for(i=0; i<N; i++){//fork N producers
		pid=fork();
		if(pid==0){ //child processes
			while(flag==1){ //loop producer code
				//printf("Im child process with pid: %d\n", getpid());
				/*----------producer code----------*/

				random_line= rand()%lines; //read random line from file
				if(random_line==0) random_line= rand()%lines; //there is no line 0
			
		
				
				prodBuffer.sem_num=0; prodBuffer.sem_op=-1; prodBuffer.sem_flg=0;//down producer sem
				if((semControl= semop(prodSem, &prodBuffer,1)) < 0){
					perror("semop");
					exit(3);
				}
				
				for(j=0;j<random_line;j++){ //overwrite entries and saves only the line chosen
					fgets(msg,sizeof msg,fp1); //reads line and saves output in msg
				}
				//printf("%s\n",msg);

     			sprintf(buffer, "%d ", getpid());//put pid in the beginning of buffer
     			strcpy(buffer2, msg); //put msg in buffer 2
     			strcat(buffer, buffer2);//concatenate strings
				strcpy(shmemP,buffer); //write to shared mem

				
				//get message from consumer
				strcpy(get_back,shmemC);//write text from shmem to temp buffer
				printf("Message is %s and my pid is: %d\n",get_back, getpid());//print capitalized text with its pid and current process pid
				strtok(get_back," "); //split string in " " and save in get_back
				comp=atoi(get_back); //string to int
				
				
				if(getpid()==comp){ //if pid matches
					pid_match++;
					printf("PID MATCH IS %d\n",pid_match);
				}
				if(strcmp(shmemC,"$$$")==0){//Before Exiting we have to up consumer sem
        			printf("Child with pid %d received Terminating message!\n",getpid());
          			flag=0;//stop loops
        		}
       			
				consBuffer.sem_num=0; consBuffer.sem_op=1; consBuffer.sem_flg=0; //up consumer sem
				if((semControl=semop(consSem, &consBuffer,1)) < 0){
					perror("semop");
					exit(4);
				}
		
        		if(flag==0){ //Terminating now!
        			printf("Child with pid %d Terminating!\n",getpid());
          			exit(pid_match);//pass matches to wait
        		}
			}
				exit(0);
		}
		if(pid < 0){//fork fail
			perror("fork");
			exit(5);
		}
	}
	/*-------------consumer code------------*/

		for(try=0; try < K+N; try++){

			consBuffer.sem_num=0; consBuffer.sem_op=-1; consBuffer.sem_flg=0;//down consumer sem
			if((semControl=semop(consSem, &consBuffer,1)) < 0){
				perror("semop");
				exit(6);
			}
			
			//printf("Im parent process with pid: %d\n", getpid());
			strcpy(to_cap,shmemP);//get text from shmem to buffer

			g=0;
			while(to_cap[g] != '\0'){
				to_cap[g]=(toupper(to_cap[g])); //capitalize text
				g++;
			}
			
			strcpy(shmemC, to_cap);//write output back to shmem
			
			
     		if(try>=K && try <K+N){ //The last loops deliver the Terminating messages at the child processes
				printf("Parent Process Terminating message sent!\n");
				strcpy(shmemC,"$$$");//$$$ is the message to terminate the child processes
			}

			prodBuffer.sem_num=0; prodBuffer.sem_op=1; prodBuffer.sem_flg=0;//up producer sem
			if((semControl= semop(prodSem, &prodBuffer,1)) < 0){
				perror("semop");
				exit(7);
			}
	}
	/*---------------------------------------*/
	//Wait for child processes to exit before deleting any shared resources
	int returned,matches=0;
	pid_t child_pid;

	for(temp_iterator=0;temp_iterator<N;temp_iterator++){
		child_pid = wait(&returned);//collect match
		returned=returned>>8; //returned shown in decimal so we convert
		matches=matches+returned;//add matches
		printf("Child with pid: %d , exited with value: %d\n", child_pid , returned);//print process stats
	}


	printf("Finished with: %d Producers %d steps and %d total pid matches\n",N,K,matches);//print total stats

	//detach shmem
	shmdt(shmemP);
	shmdt(shmemC);

	//delete sems and shmem
	if(shmctl(ShMidP, IPC_RMID,(struct shmid_ds *)0) < 0){
		perror("shmctl");
		exit(8);
	}

	if(shmctl(ShMidC, IPC_RMID,(struct shmid_ds *)0) < 0){
		perror("shmctl");
		exit(9);
	}

	if((semControl=semctl(prodSem,0,IPC_RMID,0)) <0){
		perror("semctl");
		exit(10);
	}
	if((semControl=semctl(consSem,0,IPC_RMID,0)) <0){
		perror("semctl");
		exit(11);
	}

	return 0;
}
