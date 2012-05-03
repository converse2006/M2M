#ifndef SET_EVENT_H
#define SET_EVENT_H

#include "SET.h"

/* SET device event */
void SET_event_munmap(uint32_t, uint32_t);
void SET_event_fork(int);
void SET_event_start_dalvik(char *);
void SET_event_method_entry(int);
void SET_event_method_exit(int);
//tianman
void SET_event_jni_entry(void);
void SET_event_jni_exit(void);

/* QEMU event */
void SET_event_branch(uint32_t, uint32_t);
void SET_event_syscall(uint32_t);
void SET_event_interrupt(uint32_t);

/* Accessing interanl variable. */
int  getCurrentPID();

//tianman
extern SETProcess *now_process;
char pid_name_base[512][32];
int pid_base[512][32];
char* now_name;
char process_name[PROCESS_NAME_LEN];

#endif
