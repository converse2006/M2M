#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include "SET.h"
#include "SET_ctrl.h"

#define SET_CTRL_PORTNO 60000//5504

//chiaheng
//------------------------------------------------------------------------------
#define DEBUG           1
#define DETAILED_DEBUG  0

/* The following two variables should be sync with vpmu.h */
#define MAX_VPMU_SUPPORT_EVENT  256
#define CUR_VPMU_SUPPORT_EVENT  46
#define DUMMY_EVENT_CFG 127 // MAX value of a signed char.

#ifdef DEBUG
#define SIMPLE_DBG(str,...) fprintf(stderr, str,##__VA_ARGS__);\
							fflush(stderr);
#else
#define SIMPLE_DBG(str,...) {}
#endif

pthread_mutex_t mutex_sync_perf_mntr = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  perf_mntr_cond       = PTHREAD_MUTEX_INITIALIZER;

// The settings specified by the users.
unsigned int sampleIntvl= 1000;
// Determine the event, which is used as sampling interval.
int sample_event_id     = 81; // 81
// The array of event configuration indicating which events are monitored
//dumped during program execution.
// Three values, 0, 1, and 127, are stored in this array.
//  '0' means this event is not interested by the user.
//  '1' means users are interested in this event.
//  '127' means this is not a valid event id.
volatile char eventConfig[MAX_VPMU_SUPPORT_EVENT];
// Indicate the status of the sampling process.
volatile int isSamplingOn        = 0;


// Initialize the array to MAX value.
void initEventConfig ()
{
    SIMPLE_DBG("%s: Initializing the event config list.\n", __FUNCTION__);
    int i;
    for (i=0; i<MAX_VPMU_SUPPORT_EVENT; i++)
        eventConfig[i] = DUMMY_EVENT_CFG;
}

void dumpEventConfig (int num_bytes)
{
	int i;
	SIMPLE_DBG("Received %d bytes.\n", num_bytes);
	SIMPLE_DBG("Event config list: \n");
	for (i=0; i<MAX_VPMU_SUPPORT_EVENT; i++)
	{
		// Only print the status of supported events.
		if (eventConfig[i] != DUMMY_EVENT_CFG)
			SIMPLE_DBG("\t <%d>=%d\n", i, eventConfig[i]);
	}
}

//------------------------------------------------------------------------------
// Functions that access the variables from other C files (e.g., vpmu-device.c).

inline unsigned int getSampleInterval ()
{
    return sampleIntvl;
}

inline int getSampleEventID ()
{
    return sample_event_id;
}

inline int isSampling ()
{
    return isSamplingOn;
}

inline char *getEventConfigArray ()
{
    return eventConfig;
}

int isEventMonitored (int index)
{
    if (index >= MAX_VPMU_SUPPORT_EVENT || index < 0)
    {
        SIMPLE_DBG("%s: <%d> is out of array boundary (%d).",
                __FUNCTION__, index, MAX_VPMU_SUPPORT_EVENT);
        return 0;
    }
    else if (eventConfig[index] == DUMMY_EVENT_CFG)
    {
        if (DETAILED_DEBUG)
            SIMPLE_DBG("%s: <%d> is not supported event id.",
                    __FUNCTION__, index);
        return 0;
    }
    else
    {
        return (char) eventConfig[index];
    }
}
//------------------------------------------------------------------------------


static void SET_ctrl_error(const char *msg)
{
    fprintf(stderr, "SET ctrl server: error in %s\n", msg);
    pthread_exit(NULL);
}

void *SET_ctrl_thread(void *arg)
{
    /* TCP socket */
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        SET_ctrl_error("socket");
 
    /* Initail, bind to port SET_CTRL_PORTNO */
    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SET_CTRL_PORTNO);

	/* reuse socket */
	int on = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
 
    /* binding */
    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
        SET_ctrl_error("bind");
 
    /* Start listening */
    if (listen(sockfd, 5) < 0)
        SET_ctrl_error("listen");
 
    fprintf(stderr,"SET ctrl server: start\n");

    /* Wait for connect! */
    socklen_t client_len = sizeof(client_addr);
    int new_fd, num_bytes;
    char buf[256];
	//chiaheng
	char samplingStatus[256];
    SETProcess *process;

	//chiaheng fixed
    while(1)
	{
		if ((new_fd = accept(sockfd, (struct sockaddr*) &client_addr, &client_len)) < 0) {
			if (EINTR==errno) {
				continue;  /* Restart accept */
			}
			else
				SET_ctrl_error("accept");
		}

		/* Receive - Part I. */
		if ((num_bytes = read(new_fd, buf, sizeof(buf))) < 0)
			SET_ctrl_error("recv");

		SIMPLE_DBG("Received %d bytes - process type and timing model.\n", num_bytes);

		// Starting VPMU.
		set.model = (int) buf[0];
		SET_start_vpmu();

		process = SET_create_process(buf+2, (ProcessType) buf[1]);
		if (process != NULL)
			SET_attach_process(process);

		/* Initialization of the event config list. */
		initEventConfig();

		/* Receive - Part II. */
		if ((num_bytes = read(new_fd, eventConfig, CUR_VPMU_SUPPORT_EVENT) ) < 0)
			//sizeof(eventConfig))) < 0)
			SET_ctrl_error("recv 2nd data");

		dumpEventConfig(num_bytes);
		// Tell VPMU it's good for sampling.
		isSamplingOn = 1;

		// Signal the VPMU thread to tell it that everything is ready.
		pthread_mutex_lock(&mutex_sync_perf_mntr);
		pthread_cond_signal(&perf_mntr_cond);
		pthread_mutex_unlock(&mutex_sync_perf_mntr);

		/* Receive - Part III. */
		// Blocking and waiting for stop event from user.
		// Only one byte is actually used to inidicating the sampling status.
		if ((num_bytes = read(new_fd, samplingStatus, sizeof(samplingStatus))) < 0) {
			fprintf(stderr, "\t Number of bytes/Error code (%d).\n", num_bytes);
			SET_ctrl_error("recv 3nd data");
		}

		SIMPLE_DBG("Received %d bytes - stop sampling (%d).\n", num_bytes, samplingStatus[0]);
		if ( num_bytes > 0 && !samplingStatus[0] )
		{
			isSamplingOn = 0;
			SET_stop_vpmu();
		}

		close(new_fd);
	}
    close(sockfd);
    pthread_exit(NULL);
}

