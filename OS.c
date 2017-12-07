#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include "PageTable.h"
#include <time.h>

//Initializes variables
int pageNo;
int frameNo;
int frameAvailable;
int diskCounter = 0;
int SegmentId;
page_table_pointer PageTable;

//Function to allocate victim 
int victimAlocate(page_table_pointer pp){
	clock_t lastAccessedMinTime = clock();
	int k;
	int hold_k = -1;
	for(k = 0; k < pageNo;k++){
		if(pp[k].Valid){
			if(lastAccessedMinTime > pp[k].timeLastAccessed){
				lastAccessedMinTime = pp[k].timeLastAccessed;
				hold_k = k;
			}
		}
	}
	//choose victim page
	printf("Chose a victim page %d\n",hold_k);
	if(pp[hold_k].Dirty){
		printf("Victim is dirty, write out\n");
		diskCounter++;
		sleep(1);
	}
	int ret = pp[hold_k].Frame;
	pp[hold_k].Valid = 0;
	pp[hold_k].Frame = -1;
	pp[hold_k].Dirty = 0;
	pp[hold_k].Requested = 0;
	return ret;
}

//Funtion for random allocation of frames
int randomAlocate(){
	frameAvailable--;
	return (frameNo - frameAvailable - 1);
}

//Function request handler
void requestHandler(int pages,page_table_pointer pp){
	int frameHold = -1;
	int hold = -1;
	int j = 0;
	for(j = 0 ; j < pages;j++){
		if(pp[j].Requested != 0){
			printf("Process %d has requested page %d\n",pp[j].Requested,j);
			//for available free frames
			if(frameAvailable){
				frameHold = randomAlocate();
				printf("Put it in free frame %d\n",frameHold);
			}
			//for victim's frame
			else{
				frameHold = victimAlocate(pp);
				printf("Put in victim's frame %d\n",frameHold);
			}
			hold = pp[j].Requested;
			pp[j].Valid = 1;
			pp[j].Frame = frameHold;
			pp[j].Dirty = 0;
			pp[j].Requested = 0;
			break;
		}
	}
	if(hold != -1){
		diskCounter++;
		sleep(1);
		printf("Unblock MMU\n\n");
		kill(hold,SIGCONT);
	}
	else{
		//handling errors
		if(shmdt(PageTable) == -1){
			perror("ERROR: Error detaching segment");
			exit(EXIT_FAILURE);
		}
		if(shmctl(SegmentId,IPC_RMID,NULL) == -1){
			perror("ERROR: Error removing segment");
                        exit(EXIT_FAILURE);
		}
		//print final output :: no of disk accesses required
		printf("%d disk accesses required\n",diskCounter);
		exit(EXIT_SUCCESS);
	}
}

//Function for sig handler
void sigHandler(int sigNo){
	if(sigNo == SIGUSR1){
		requestHandler(pageNo,PageTable);
	}
}

//Main function
int main(int argc,char *argv[]){
	pageNo = atoi(argv[1]);
	frameNo = atoi(argv[argc-1]);
	frameAvailable = frameNo;
	key_t key = getpid();
	if((SegmentId = shmget(key,pageNo * sizeof(page_table_entry),IPC_CREAT | 0660)) == -1){
		perror("shmget");
		exit(EXIT_FAILURE);
	}
	if((PageTable = (page_table_pointer)shmat(SegmentId,NULL,0)) == NULL){
		perror("shmat");
		exit(EXIT_FAILURE);
	}
	//print shared memory key i.e PID
	printf("The shared memory key (PID) is %d\n",key);
	int i;
	for(i = 0; i < pageNo;i++){
		PageTable[i].Valid = 0;
		PageTable[i].Frame = -1;
		PageTable[i].Dirty = 0;
		PageTable[i].Requested = 0;
	}
	//Initializing PageTable
	printf("Initialized page table\n");
	if(signal(SIGUSR1,sigHandler) == SIG_ERR){
		perror("error initizialing signal handler\n");
		exit(EXIT_FAILURE);
	}
	while(1){
	
	}
}
