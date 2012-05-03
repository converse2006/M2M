#ifndef SET_CTRL_SAMPLING_H
#define SET_CTRL_SAMPLING_H

#include <pthread.h>

inline unsigned int getSampleInterval ();
inline int getSampleEventID ();
inline int isSampling ();
inline char *getEventConfigArray ();
inline int isEventMonitored (int index);

/* Get timing model used by SET. */
int getSETTimingModel();

#endif /* SET_CTRL_SAMPLING.H */
