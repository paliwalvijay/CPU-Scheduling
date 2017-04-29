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
// Implementing FCFS
pid_t spid,ipid;
typedef struct entry{
	pid_t pid;
	ttype start_time;
	ttype end_time;
	ttype run_time;
}shM;
key_t key=5678;
const int size = sizeof(struct entry);
int segment_id=shmget(key,size*21,IPC_CREAT|0666);
shM *sm = (shM *)shmat(segment_id,NULL,0);
sem_t sem,stopsem,startsem;
int semvv=0;
int count1=1;

// Process Injector
void sig_handler(int SIG){
	if(SIG==SIGUSR1){
		while(semvv==0);
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
void processWork(int id, int t){
	kill(getpid(),SIGUSR1);
	timeUsage(t);
	kill(spid,SIGUSR2);
}
pid_t createProcess(int no, int siz,shM *sm){
	pid_t pid1;
	kill(spid,SIGUSR1);
	pid1 = fork();
	if(pid1==0){
		sem_init(&sem,0,0);
		processWork(no,siz);
	}
	else{
		sm[sm[0].pid].pid = pid1;
		sm[sm[0].pid].run_time=0;
		printf("Process %d arrives. \n",sm[0].pid);
		sm[0].pid++;
	}
	return pid1;
}
void process_injector(){
	signal(SIGUSR1,sig_handler);
	signal(SIGUSR2,sig_handler);
	for(int i=1;i<=10;i++){
		pid_t pi = createProcess(i,1000/i,sm);
		if(pi==0) break;
		else continue;
	}
	while(wait(NULL)>0);
	shmdt(sm);
	exit(0);
}
// FCFS Scheduler
void sig_handler2(int SIG){
	if(SIG==SIGUSR1){
		sem_wait(&startsem);
		sm[count1].start_time=clock();
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
	int ind=1;
	sem_init(&stopsem,0,0);
	while(1){
		while((sm[0].pid)!=ind){
			clock_t endt,stt=clock();
			printf("Process %d is scheduled.\n",ind);
			kill(sm[ind].pid,SIGUSR2);
			sem_wait(&stopsem);
			endt=clock();
			sm[ind].run_time+=(endt-stt);
			sm[ind].end_time=endt;
			ind++;
		}
		if(ind>=11){
			clock_t twtt=0;
			for(int i=1;i<=10;i++){
				clock_t watt = sm[i].end_time-sm[i].start_time-sm[i].run_time;
				twtt+=watt;
				cout<<"Process No:"<<(i)<<" Process Id:"<<sm[i].pid<<" Start Time= "<<sm[i].start_time<<" Waiting Time:"<<watt<<" Run time: "<<sm[i].run_time<<" End time: "<<sm[i].end_time<<endl<<endl;
			}
			cout<<"\n\tAverage waiting time= "<<(twtt/10)<<endl;
			shmdt(sm);
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
