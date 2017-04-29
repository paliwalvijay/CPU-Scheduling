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
// Implementing Multilevel Feedback queue
pid_t spid,ipid;
typedef struct entry{
	pid_t pid;
	int burst;
	int val;
	int que_level;
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
int myquant;
int qu[4]={8*30,8*60,8*80,8*100};
int myex=0;
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
	if(t<0) return;
	int tim = int(t/8);
	usleep(tim);
}
// Actual working process
void processWork(int id, int t){
	kill(getpid(),SIGUSR1);
	//cout<<"hsad "<<myquant<<endl;
	while(myquant>qu[myex]){
		//printf("Process %d starts.\n",id);
		timeUsage(qu[myex]);
		myquant = myquant-qu[myex];
		if(myex<3)myex++;
		kill(spid,SIGUSR2);
		kill(getpid(),SIGUSR1);
	}
	timeUsage(myquant);
	printf("Process %d finishes completely.\n",id);
	kill(spid,SIGUSR2);
	exit(0);
}
//Creation of process
pid_t createProcess(int no, int siz,int lev,shM *sm){
	pid_t pid1;
	kill(spid,SIGUSR1);
	pid1 = fork();
	if(pid1==0){
		sem_init(&sem,0,0);
		myquant=siz;
		processWork(no,siz);
	}
	else{
		sm[sm[0].pid].pid = pid1;
		sm[sm[0].pid].run_time=0;
		sm[sm[0].pid].val = 1;
		sm[sm[0].pid].burst = siz;
		sm[sm[0].pid].que_level = lev;
		printf("Process %d arrives. \n",sm[0].pid);
		sm[0].pid++;
	}
	return pid1;
}
//Process Injector
void process_injector(){
	signal(SIGUSR1,sig_handler);
	signal(SIGUSR2,sig_handler);
	for(int i=1;i<=10;i++){
		pid_t pi = createProcess(i,1000/i,1,sm);
		if(pi==0) break;
		else continue;
	}
	while(wait(NULL)>0);
	shmdt(sm);
	exit(0);
}
// Multilevel Feedback Queue Scheduler
void sig_handler2(int SIG){
	if(SIG==SIGUSR1){
		sem_wait(&startsem);
		sm[count1].start_time=clock();
		count1++;
		sem_post(&startsem);
	}
	else if(SIG==SIGUSR2){
		//cout<<"Called\n";
		sem_post(&stopsem);
	}
}
void process_scheduler(){
	signal(SIGUSR1,sig_handler2);
	signal(SIGUSR2,sig_handler2);
	int done=0;
	while(1){
		int ind=1;
		sem_init(&stopsem,0,0);
		int prevsch=-1;
		while((sm[0].pid)!=done){
			int sch=-1,minb=1000;
			for(int l=1;l<=4;l++){
				sch=-1;minb=1000000000;
				for(int i=1;i<sm[0].pid;i++){
					if((sm[i].que_level==l) && (sm[i].burst>0) && (sm[i].burst<minb)){
						minb = sm[i].burst;
						sch = i;
					}
				}
				if(sch!=-1) break;
			}	
			if(sch==-1) break;
			if(prevsch!=sch){
				if(prevsch!=-1){
					if(sm[prevsch].burst>0){
						//printf("Process %d with remaining burst time %d is pre-empted.\n",prevsch,sm[prevsch].burst);
					}
				}
				printf("Process %d with burst time %d and in level %d starts.\n",sch,minb,sm[sch].que_level);
			}
			prevsch=sch;
			//sm[sch].val=0;
			clock_t endt,stt=clock();
			kill(sm[sch].pid,SIGUSR2);
			sem_wait(&stopsem);
			sm[sch].burst-=qu[sm[sch].que_level-1];
			//cout<<sm[sch].burst<<endl;
			if(sm[sch].burst<=0) done++;
			else{
				if(sm[sch].que_level<4){
					sm[sch].que_level++;	
					printf("Process %d is sent to level %d. Remaining burst time = %d.\n",sch,sm[sch].que_level,sm[sch].burst);
				}
			}
			//cout<<"Done="<<done<<endl;
			endt=clock();
			sm[sch].run_time+=(endt-stt);
			sm[sch].end_time=endt;
			//done++;
		}
		if(done>=10){
			clock_t twtt=0;
			for(int i=1;i<=10;i++){
				clock_t watt = sm[i].end_time-sm[i].start_time-sm[i].run_time;
				twtt+=watt;
				cout<<"Process No:"<<(i)<<" Process Id:"<<sm[i].pid<<" Start Time= "<<sm[i].start_time<<" Waiting Time:"<<watt<<" Run time: "<<sm[i].run_time<<" End time: "<<sm[i].end_time<<endl<<endl;
			}
			cout<<"\n\tAverage waiting time= "<<(twtt/10)<<endl;
			shmdt(sm);
			break;
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
