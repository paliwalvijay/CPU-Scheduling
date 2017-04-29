#include<iostream>
#include<pthread.h>
#include<stdlib.h>
#include<stdio.h>
#include<fstream>
#include<queue>
#include<unistd.h>
#include<algorithm>
#include<vector>
#include<semaphore.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<signal.h>
#include<map>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<list>
#include<time.h>
#define ttype double
using namespace std;
// Implementing Round Robin
pid_t spid,ipid;
typedef struct entry{
	pid_t pid;
	int quant;
	ttype start_time;
	ttype end_time;
	ttype run_time;
}shM;
key_t key=5678;
const int size = sizeof(struct entry);
int segment_id=shmget(key,size*21,IPC_CREAT|0666);
shM *sm = (shM *)shmat(segment_id,NULL,0);
sem_t sem,stopsem,startsem,notesem;
int semvv=0;
int count1=1;
int myquant;
int qu=500;
// Signal Handler
void sig_handler(int SIG){
	if(SIG==SIGUSR1){
		while(semvv==0);
		semvv=0;
	}
	else if(SIG==SIGUSR2){
		//sem_post(&sem);
		semvv=1;
	}
}
void timeUsage(float t){
	int tim = int(t);
	usleep(tim);
}
// Actual working process
void processWork(int id, int t){
	kill(getpid(),SIGUSR1);
	while(myquant>qu){
		printf("Process %d starts.\n",id);
		timeUsage(qu);
		myquant = myquant-qu;
		printf("Process %d finishes partially.\n",id);
		kill(spid,SIGUSR2);
		kill(getpid(),SIGUSR1);
	}
	printf("Process %d starts.\n",id);
	timeUsage(myquant);
	printf("Process %d finishes completely.\n",id);
	kill(spid,SIGUSR2);
	exit(0);
}
//Creation of process
pid_t createProcess(int no, int siz,shM *sm){
	pid_t pid1;
	//cout<<"Calling signal\n\n";
	//sem_wait(&notesem);
	kill(spid,SIGUSR1);
	//timeUsage(10);
	//sem_post(&notesem);
	pid1 = fork();
	if(pid1==0){
		sem_init(&sem,0,0);
		myquant=siz;
		processWork(no,siz);
	}
	else{
		sm[sm[0].pid].pid = pid1;
		sm[sm[0].pid].run_time=0;
		sm[sm[0].pid].quant = siz;
		printf("Process %d arrives. \n",sm[0].pid);
		sm[0].pid++;
	}
	return pid1;
}
//Process Injector
void process_injector(){
	signal(SIGUSR1,sig_handler);
	signal(SIGUSR2,sig_handler);
	sem_init(&notesem,0,1);
	for(int i=1;i<=10;i++){
		//timeUsage(10);
		pid_t pi = createProcess(i,1000/i,sm);
		if(pi==0) break;
		else continue;
	}
	while(wait(NULL)>0);
	shmdt(sm);
	exit(0);
}
// Round Robin Scheduler
void sig_handler2(int SIG){
	//cout<<count1<<endl;
	if(SIG==SIGUSR1){
		sem_wait(&startsem);
	//	cout<<count1<<endl;
		sm[count1].start_time=clock();
		if(count1>1 && sm[count1-1].start_time>sm[count1].start_time) cout<<"Error!\n\n\n\n\n";
		count1++;
		sem_post(&startsem);
	}
	else if(SIG==SIGUSR2){
		sem_post(&stopsem);
	}
}
void process_scheduler(){
	signal(SIGUSR1,sig_handler2);
	signal(SIGUSR2,sig_handler2);
	while(1){
		int ind=1;
		sem_init(&stopsem,0,0);
		while((sm[0].pid)!=ind){
			if(sm[ind].quant==0){ind++; continue;}
			else{
				if(sm[ind].quant>qu){sm[ind].quant-=qu;}
				else sm[ind].quant=0;
			}
			clock_t endt,stt=clock();
			kill(sm[ind].pid,SIGUSR2);
			sem_wait(&stopsem);
			endt=clock();
			sm[ind].run_time+=(endt-stt);
			sm[ind].end_time=endt;
			ind++;
		}
		if(ind>=11){
			int live=0;
			for(int i=1;i<=10;i++){
				if(sm[i].quant>0){live++;break;}
			}
			if(live==0){
			clock_t twtt=0;
			for(int i=1;i<=10;i++){
				clock_t watt = sm[i].end_time-sm[i].start_time-sm[i].run_time;
				twtt+=watt;
				cout<<"Process No:"<<(i)<<" Process Id:"<<sm[i].pid<<" Start Time= "<<sm[i].start_time<<" Waiting Time:"<<watt<<" Run time: "<<sm[i].run_time<<" End time: "<<sm[i].end_time<<endl<<endl;
			}
			cout<<"\n\tAverage waiting time= "<<(twtt/10)<<endl;
			shmdt(sm);
			}
			else continue;
		}
	}
}

int main(){
	// Code for shared segment id
	sem_init(&startsem,0,1);
	sm[0].pid=1;
	spid = fork();  //Process Scheduler
	if(spid!=0) ipid = fork();  //Process Injector
	if(spid==0){
		//Process Scheduler
		process_scheduler();
	}
	else if(ipid==0 && spid!=0){
		//Process Injector
		process_injector();
	}
	else{
		while(wait(NULL)>0);
		shmdt(sm);
		shmctl(segment_id,IPC_RMID,NULL);
	}
}
