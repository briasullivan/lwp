/**
 * @file
 * CPE 453 Spring 2012
 * -------------------
 *
 * Project #2: LWP
 *
 * Last Modified: April 19, 2011   Pacific Daylight Time 2010
 * @author Timothy Asp
 * Copyright (C) 2011 Timothy P. Asp. All rights reserved.
 */

#include "lwp.h"
#include "smartalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int round_robin_scheduler();
schedfun scheduler = round_robin_scheduler;

/* parrent process stack pointer. the "real" sp */
ptr_int_t real_sp;

/* process counter */
int lwp_procs = 0;

/* current process index */
int lwp_running = 0;
lwp_context lwp_ptable[LWP_PROC_LIMIT];

extern int new_lwp(lwpfun fun, void * args, size_t stack_size) {

	if (lwp_procs > LWP_PROC_LIMIT) {
		return -1;
	}

	/* pid is set to whatever, so I'm just using the number of processes 
	 * at that time +2 */
	lwp_ptable[lwp_procs].pid = lwp_procs+2;
	lwp_ptable[lwp_procs].stack = malloc (stack_size * sizeof(ptr_int_t));
	lwp_ptable[lwp_procs].stacksize = stack_size;

	/* set the TOS */
	lwp_ptable[lwp_procs].stack[0] = 0xAAAAAAAA;
	
	/* push the functions args on the stack */
	lwp_ptable[lwp_procs].stack[stack_size-1] = (ptr_int_t) args; 

	/* push the return address for the function to follow when it completes */
	lwp_ptable[lwp_procs].stack[stack_size-2] = (ptr_int_t) lwp_exit;
	
	/* push the address of the function to call */
	lwp_ptable[lwp_procs].stack[stack_size-3] = (ptr_int_t) fun;

	/** push the "old" base pointer on the stack, doesn't matter what it is, 
	 * DEAD0000 makes it easy to debug and remember 
	 **/
	lwp_ptable[lwp_procs].stack[stack_size-4] = 0xDEAD0000;

	/* push the registers onto the stack now.  This is needed to trick the
	 * SAVE_STATE and RESTORE_STATE macros to think that it's not a new stack,
	 * but just one that's been paused. The values don't matter since they'll 
	 * just be overwritten by the macros, they're just easy to notice
	 */

	/* eax */
	lwp_ptable[lwp_procs].stack[stack_size-5] = 0x04;
	/* ebx */
	lwp_ptable[lwp_procs].stack[stack_size-6] = 0x08;
	/* ecx */
	lwp_ptable[lwp_procs].stack[stack_size-7] = 0x0B;
	/* edx */
	lwp_ptable[lwp_procs].stack[stack_size-8] = 0x0E;
	/* esi */
	lwp_ptable[lwp_procs].stack[stack_size-9] = 0x12;
	/* edi */
	lwp_ptable[lwp_procs].stack[stack_size-10] = 0x16;

	/* set the stack pointer and the base pointer to the "old" base pointer */
	lwp_ptable[lwp_procs].stack[stack_size-11] = 
	 (ptr_int_t) &(lwp_ptable[lwp_procs].stack[stack_size-4]);
	lwp_ptable[lwp_procs].sp = &(lwp_ptable[lwp_procs].stack[stack_size-11]);

	/* incread the process counter */
	lwp_procs++;

	/* return the process id for the newly created process */
	return lwp_ptable[lwp_procs-1].pid;
}

/* Terminate the LWP */
extern void lwp_exit() {
	/* store the exited lwp index as prev_lwp */
	int offset, prev_lwp = lwp_running;

	/* get the next process to schedule */
	lwp_running = scheduler();

	/* make sure the scheduler didn't error */
	if (lwp_running >= lwp_procs || lwp_running < 0) {
		lwp_stop();
	}

	/* move the process table up */
	for (offset=1; (offset + prev_lwp) < lwp_procs; offset++) {
      lwp_ptable[(offset-1) + prev_lwp] = lwp_ptable[offset+prev_lwp];
	}

	/* a process exited, so the number goes down */
	lwp_procs--;

	/* if there isn't any processes left, clean up and go to stop */
	if (lwp_procs <= 0) {
		lwp_stop();
	}

	/** because we shifted the table down, we need to update the index 
	 *  of the current lwp, but only if its not the first one
	 **/
	if (lwp_running != 0)
		lwp_running--;

	/* load up the next process */
	SetSP(lwp_ptable[lwp_running].sp);
	/* and run it */
	RESTORE_STATE();
}

/* return the current lwp pid */
extern int lwp_getpid() {
	return lwp_ptable[lwp_running].pid;
}

/* pause the current process and start up the next one */
extern void lwp_yield() {
	/* save the paused lwp's stack pointer back into the table */
	SAVE_STATE();
	GetSP(lwp_ptable[lwp_running].sp);

	/* get the next scheduled process */
	lwp_running = scheduler();

	if (lwp_running >= lwp_procs || lwp_running < 0) {
		fprintf(stderr, "schedule problem");
		exit(-1);
	}

	/* load in the new processes stack information into registers */
	SetSP(lwp_ptable[lwp_running].sp);
	/* start the new lwp */
	RESTORE_STATE();
}

/* start the lwp */
extern void lwp_start() {

	/* save the parents "real" stack pointer so we can get back
	 * to it at the end 
	 * The real_sp is used by lwp_stop */
	SAVE_STATE();
	GetSP(real_sp);

	/* if there are no processes, return immediatly */
	if (lwp_procs <= 0) {
		lwp_stop();
	}

	/* load in the new processes stack information into registers */
	SetSP(lwp_ptable[lwp_running].sp);
	/* and start that function up */
	RESTORE_STATE();
}

/* restore the original stack pointer and control back to the parent */
extern void lwp_stop() {
	/* will the "real" sp please stand up? */
	SetSP(real_sp);
	/* lets restore some normality */
	RESTORE_STATE();
}

/* set the scheduler function */
extern void lwp_set_scheduler(schedfun sched) {
	/* does the schedule function exist or not null? */
	if (sched != NULL) {
		scheduler = sched;
	} else {
		/* if its not set the scheduler to do round robin */
		scheduler = round_robin_scheduler;
	}
}

/* "dumb" round robin scheduler, chooses for the next process in stack */
int round_robin_scheduler() {
	int new_lwp = lwp_running;
	
	/* if the lwp is the last one */
	if (new_lwp == lwp_procs-1) {
		new_lwp = 0;
	} else {
		/* otherwise just incriment the current lwp index */
		new_lwp++;
	}

	return new_lwp;
}

