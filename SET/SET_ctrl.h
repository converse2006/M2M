#ifndef SET_CTRL_H
#define SET_CTRL_H

void *SET_ctrl_thread(void *);

//evo0209
void dumpEventConfig(int);
extern pthread_mutex_t mutex_sync_perf_mntr;
extern pthread_cond_t  perf_mntr_cond;
extern volatile int isSamplingOn;   

#endif
