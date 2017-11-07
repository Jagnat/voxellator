#ifndef _VOX_JOBS_H_
#define _VOX_JOBS_H_

struct Job
{
	void (*jobProc)(void*);
	void (*completionProc)(void*);
	void *args;
	volatile int done;
	int priority;
	Job *next;
};

struct JobManager
{
	Job *jobHeap;
	int jobsQueued;
	int heapSize;

	Job *runningJobs;
	Job *freeJobs; // Free linked list
	int maxThreads;
	int jobsActive;
};

void initJobSystem(int maxThreads);
void processJobs();
void addJob(Job job);

#endif _VOX_JOBS_H_