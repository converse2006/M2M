//#include "../SET/SET.h"
//#include "../SET/SET_event.h"
#include "./SET/SET_event.h"
// For usleep.
#include <unistd.h>

/*----------------------------------------------------------------------------
 * A separate thread used to dump the selected events periodically.
 * The configuration of the whole sampling mechanism is controlled by
 *SET_Control tool, which will get the events and sample interval from users.
 *
 */
pthread_t perf_mntr_thread;

extern pthread_mutex_t mutex_sync_perf_mntr;
extern pthread_cond_t  perf_mntr_cond;
// Mapping from event configuration list to event id recognized by the
//function: vpmu_hardware_event_select.
int vpmu_event_id_table [CUR_VPMU_SUPPORT_EVENT] = {
// Performance Events To Be Monitored.
// Instructions related events.
TOT_INST,
USR_INST,
SVC_INST,
IRQ_INST,
REST_INST,
TOT_LD_ST_INST,
TOT_USR_LD_ST_INST,
TOT_SVC_LD_ST_INST,
TOT_IRQ_LD_ST_INST,
TOT_REST_LD_ST_INST,
TOT_LD_INST,
TOT_USR_LD_INST,
TOT_SVC_LD_INST,
TOT_IRQ_LD_INST,
TOT_REST_LD_INST,
TOT_ST_INST,
TOT_USR_ST_INST,
TOT_SVC_ST_INST,
TOT_IRQ_ST_INST,
TOT_REST_ST_INST,
PC_CHANGE,
// Branch related events.
BRANCH_INST,
BRANCH_PREDICT_CORRECT,
BRANCH_PREDICT_WRONG,
// Cache realted events.
// Please make sure you have turned on the cache simulation.,
DCACHE_ACCESSES_CNT,
DCACHE_WRITE_MISS,
DCACHE_READ_MISS,
DCACHE_MISS,
ICACHE_ACCESSES_CNT,
ICACHE_MISS,
// Memory acess.
SYSTEM_MEM_ACCESS,
IO_MEM_ACCESS,
// Estimated execution time,
ELAPSED_CPU_CYC,
ELAPSED_EXE_TIME,
ELAPSED_PIPELINE_CYC,
ELAPSED_PIPELINE_EXE_TIME,
ELAPSED_SYSTEM_MEM_CYC,
ELAPSED_SYSTEM_MEM_ACC_TIME,
ELAPSED_IO_MEM_CYC,
ELAPSED_IO_MEM_ACC_TIME,
// Energy events,
MPU_ENERGY,
//DSP1_ENERGY,
//DSP2_ENERGY,
LCD_ENERGY,
AUDIO_ENERGY,
NET_ENERGY,
GPS_ENERGY,
TOTAL_ENERGY
};

// String for performance events.
char VPMU_EVENT_DESP [][30] = {
"TOT_INST",
"USR_INST",
"SVC_INST",
"IRQ_INST",
"REST_INST",
"TOT_LD_ST_INST",
"TOT_USR_LD_ST_INST",
"TOT_SVC_LD_ST_INST",
"TOT_IRQ_LD_ST_INST",
"TOT_REST_LD_ST_INST",
"TOT_LD_INST",
"TOT_USR_LD_INST",
"TOT_SVC_LD_INST",
"TOT_IRQ_LD_INST",
"TOT_REST_LD_INST",
"TOT_ST_INST",
"TOT_USR_ST_INST",
"TOT_SVC_ST_INST",
"TOT_IRQ_ST_INST",
"TOT_REST_ST_INST",
"PC_CHANGE",
"BRANCH_INST",
"BRANCH_PREDICT_CORRECT",
"BRANCH_PREDICT_WRONG",
// Cache realted events.
// Please make sure you have turned on the cache simulation.,
"DCACHE_ACCESSES_CNT",
"DCACHE_WRITE_MISS",
"DCACHE_READ_MISS",
"DCACHE_MISS",
"ICACHE_ACCESSES_CNT",
"ICACHE_MISS",
// Memory acess.
"SYSTEM_MEM_ACCESS",
"IO_MEM_ACCESS",
// Estimated execution time,
"ELAPSED_CPU_CYC",
"ELAPSED_EXE_TIME",
"ELAPSED_PIPELINE_CYC",
"ELAPSED_PIPELINE_EXE_TIME",
"ELAPSED_SYSTEM_MEM_CYC",
"ELAPSED_SYSTEM_MEM_ACC_TIME",
"ELAPSED_IO_MEM_CYC",
"ELAPSED_IO_MEM_ACC_TIME",
// Energy events,
"MPU_ENERGY",
//DSP1_ENERGY,
//DSP2_ENERGY,
"LCD_ENERGY",
"AUDIO_ENERGY",
"NET_ENERGY",
"GPS_ENERGY",
"TOTAL_ENERGY"
};
#define DEBUG_PERF_MONITORING   1

#ifdef DEBUG_PERF_MONITORING
    #define DEBUG_PM(str,...) fprintf(stderr, str,##__VA_ARGS__); \
                                fflush(stderr);
#else
    #define DEBUG_PM(str,...) {}
#endif

extern volatile char eventConfig[CUR_VPMU_SUPPORT_EVENT];

/* Dump current networking mechanism for power calculation.
 *
 * Three differnt scenarios:
 * 1: indicating the network is driving by Wifi.
 * 2: indicating the network is driving by 3G network.
 * 0: indicating the network is off.
 */
void samplingPeripheralPower (void *opaque)
{
    FILE *power_fp;

    power_fp=fopen("/tmp/power_dump.txt","w+");
    fprintf(power_fp, "Total-power LCD CPU Wifi 3G GPS Audio\n");
    DEBUG_PM("VPMU: peripheral power sampling staring\n");
    DEBUG_PM("Sampling..\n");

    while ( isSampling() )
    {
        if(adev_stat.net_mode == 1) { //Wifi mode
            fprintf(power_fp, "%d %d %d %d 0 %d %d\n", vpmu_vpd_total_power(),
                      vpmu_vpd_lcd_power(), vpmu_vpd_mpu_power(), vpmu_vpd_net_power(),
                      vpmu_vpd_gps_power(), vpmu_vpd_audio_power());
        }
        else if(adev_stat.net_mode == 2) { //3G mode
            fprintf(power_fp, "%d %d %d 0 %d %d %d\n", vpmu_vpd_total_power(),
                      vpmu_vpd_lcd_power(), vpmu_vpd_mpu_power(), vpmu_vpd_net_power(),
                     vpmu_vpd_gps_power(), vpmu_vpd_audio_power());
        }
        else { //off
            fprintf(power_fp, "%d %d %d 0 0 %d %d\n", vpmu_vpd_total_power(),
                      vpmu_vpd_lcd_power(), vpmu_vpd_mpu_power(),
                      vpmu_vpd_gps_power(), vpmu_vpd_audio_power());
        }
        //usleep(300000);
		//evo0209
		usleep(1000000);
        fflush(power_fp);
    }
    DEBUG_PM("Sampling finished...\n\n");
    fclose (power_fp);

}

// Use the buffering mechanism provided by the OS.
#define USE_FS_BUFFER       0

#if !USE_FS_BUFFER  /* USE a large buffer for trace data. */

void samplingProcessorEvents (void *opaque)
{
#define BUFFER_SIZE         50*1024+1024
    // Used for general sample file.
    unsigned int previous_value;
    unsigned int cur_value  = 0;
    int firstSample         = 1;

    // Get the parameters from SET.
    unsigned int sampleIntvl;
    // Indicate the current running process.
    int cur_pid;
    // Determine the event, which is used as sampling interval.
    int sample_event_id;
    int sizeofEventIdTable  = sizeof(vpmu_event_id_table)/sizeof(int);

    int bits                = sizeof(int)*8;
    int i;
    /* The sequence id for dumped trace data tuples. */
    unsigned int j = 1; 
    unsigned int k = 1; // frequency = 10us

    /* Two sampling raw files. */
    /* 1. General sampling mechanism is applied. 
     * User specify the events to be monitored.
     */
    FILE *vpmu_samples = fopen("/tmp/vpmu_samples.txt", "w+");
    /* 2. Thread profiling.
     * Dump the PID and EET periodically.
     */
    FILE *thread_profile = fopen("/tmp/thread_profile.txt", "w+"); 

    // Used for thread profiling.
    unsigned int       threadSampleIntl = 1000;
    uint32_t last_EET_ns      = 0;
    uint32_t cur_EET_ns       = 0;
    unsigned long vpmu_samples_idx      = 0;
    unsigned long thread_profile_idx    = 0;

    char *vpmu_samples_buf = malloc (sizeof(char)*BUFFER_SIZE);
    char *thread_profile_buf = malloc (sizeof(char)*BUFFER_SIZE);

    while ( isSampling() )
    {
        if (firstSample)
        {
            DEBUG_PM("*Sampling..\n");

            /* Init for general sample file. */
            // Update the updated status.
            sampleIntvl     = getSampleInterval();
            sample_event_id = getSampleEventID();
            previous_value  = vpmu_hardware_event_select((uint32_t)sample_event_id, (VPMU*) opaque);
            firstSample     = 0;
            vpmu_samples_idx    = 0;
            thread_profile_idx  = 0;

            vpmu_samples_idx = sprintf(vpmu_samples_buf, "Sample id (%d), interval (%u), starting value (%u).\n",
                        sample_event_id, sampleIntvl, previous_value);

            vpmu_samples_idx += sprintf(vpmu_samples_buf+vpmu_samples_idx, "Seq_id\tPID\tProcess_name\tEET(ns)");

            for (i=0; i< sizeofEventIdTable; i++)
                if ( isEventMonitored(i) == EVENT_ON )
                    vpmu_samples_idx += sprintf(vpmu_samples_buf+vpmu_samples_idx, "\t%s", VPMU_EVENT_DESP[i]);
            vpmu_samples_idx += sprintf(vpmu_samples_buf+vpmu_samples_idx, "\n");

            /* Init for thread profiling. */
            last_EET_ns     = vpmu_estimated_execution_time_ns();

            thread_profile_idx = sprintf(thread_profile_buf, "PID\tEET\n");
        }
        else
        {
            /* Sampling routine for general sample file. */
            cur_value = vpmu_hardware_event_select((uint32_t)sample_event_id, (VPMU*) opaque);
    
//            fprintf (stderr, "current value = %u, previous value =%u.\n ", cur_value, previous_value);
            // Check if the sampling interval is reached.
            if ( (cur_value - previous_value) > sampleIntvl )
            {
                // Get the current running thread id.
                cur_pid             = getCurrentPID();
				
				//tianman test
				//for(i=0;i<512;i++){
				//	if(pid_base[i][0]==cur_pid)
						//fprintf(vpmu_samples, "\n pid %d name %s \n", cur_pid, pid_name_base[i]);
						//printf("\n pid %d name %s \n", cur_pid, pid_name_base[i]);
				//}

                // Dump the Seq_id, EET, and PID.
                vpmu_samples_idx += sprintf(vpmu_samples_buf+vpmu_samples_idx, 
                                "%u\t%d\t%s\t%u", j, cur_pid, pid_name_base[cur_pid], vpmu_estimated_execution_time_ns());

                for (i=0; i< sizeofEventIdTable; i++)
                {
//                    DEBUG_PM("EVENT [%d]=%d, on/off(%d), value=", i,
//                                    vpmu_event_id_table[i], isEventMonitored(i) );

                    // Do user want to observe this event?
                    if ( isEventMonitored(i) == EVENT_ON )
                    {
                        vpmu_samples_idx += sprintf(vpmu_samples_buf+vpmu_samples_idx, 
                                "\t%u", vpmu_hardware_event_select(
                                    (uint32_t)vpmu_event_id_table[i], (VPMU*) opaque));
//                        DEBUG_PM("%lu !.\n", vpmu_hardware_event_select(
//                                    (uint32_t)vpmu_event_id_table[i], (VPMU*) opaque));
                    }
                    else
                    {
//                        DEBUG_PM("%lu.\n", vpmu_hardware_event_select(
//                                    (uint32_t)vpmu_event_id_table[i], (VPMU*) opaque));
                    }


                }
//                DEBUG_PM("\n");
                vpmu_samples_idx += sprintf(vpmu_samples_buf+vpmu_samples_idx, "\n");
                previous_value = cur_value;
                j++;
            }
            
            /* Sampling routine for thread profiling. */
            cur_EET_ns = vpmu_estimated_execution_time_ns();
            if ( cur_EET_ns - last_EET_ns > threadSampleIntl ) 
            {
                // Get the current running thread id.
                cur_pid             = getCurrentPID();
                //thread_profile_idx += sprintf(thread_profile_buf+thread_profile_idx, 
                //                       "%u\t%u\n", cur_pid, cur_EET_ns-last_EET_ns);
                thread_profile_idx += sprintf(thread_profile_buf+thread_profile_idx, 
                                       "%u\t%s\t%u\n", cur_pid, pid_name_base[cur_pid], cur_EET_ns-last_EET_ns);
                last_EET_ns = cur_EET_ns;
                k++;
            }
        }

        if (vpmu_samples_idx > BUFFER_SIZE - 1024)
        {
            fwrite (vpmu_samples_buf, 1, vpmu_samples_idx, vpmu_samples);
            vpmu_samples_idx = 0;
        }
        if (thread_profile_idx > BUFFER_SIZE - 1024)
        {
            fwrite (thread_profile_buf, 1, thread_profile_idx, thread_profile);
            thread_profile_idx = 0;
        }
    }

    if (vpmu_samples_idx)
        fwrite (vpmu_samples_buf, 1, vpmu_samples_idx, vpmu_samples);
    if (thread_profile_idx)
        fwrite (thread_profile_buf, 1, thread_profile_idx, thread_profile);

    DEBUG_PM("Sampling finished...\n\n");

    fflush(vpmu_samples);
    fclose (vpmu_samples);
    fflush(thread_profile);
    fclose(thread_profile);
}

#else /* USE the file buffering mechanism provided by OS. */

void samplingProcessorEvents (void *opaque)
{
    unsigned int previous_value;
    unsigned int cur_value  = 0;
    int firstSample         = 1;

    // Get the parameters from SET.
    unsigned int sampleIntvl;
    // Indicate the current running process.
    int cur_pid;
    // Determine the event, which is used as sampling interval.
    int sample_event_id;
    int sizeofEventIdTable  = sizeof(vpmu_event_id_table)/sizeof(int);

    // Used for thread profiling.
    unsigned int       threadSampleIntl = 1000;
    unsigned long long last_EET_ns      = 0;
    unsigned long long cur_EET_ns       = 0;

    int bits                = sizeof(int)*8;
    int i;
    /* The sequence id for dumped trace data tuples. */
    unsigned int j = 1; 
    unsigned int k = 1; // frequency = 10us

    /* Two sampling raw files. */
    /* 1. General sampling mechanism is applied. 
     * User specify the events to be monitored.
     */
    FILE *vpmu_samples = fopen("/tmp/vpmu_samples.txt", "w+");
    /* 2. Thread profiling.
     * Dump the PID and EET periodically.
     */
    FILE *thread_profile = fopen("/tmp/thread_profile.txt", "w+"); 

    while ( isSampling() )
    {
        if (firstSample)
        {
            DEBUG_PM("Sampling..\n");

            /* 1. Init for general sample file. */
            // Update the updated status.
            sampleIntvl     = getSampleInterval();
            sample_event_id = getSampleEventID();
            previous_value  = vpmu_hardware_event_select((uint32_t)sample_event_id, (VPMU*) opaque);
            firstSample     = 0;

            fprintf(vpmu_samples, "Sample id (%d), interval (%u), starting value (%u).\n",
                        sample_event_id, sampleIntvl, previous_value);

            fprintf(vpmu_samples,"Seq_id\tPID\tEET(ns)");

            for (i=0; i< sizeofEventIdTable; i++)
                if ( isEventMonitored(i) == EVENT_ON )
                    fprintf(vpmu_samples, "\t%s", VPMU_EVENT_DESP[i]);
            fprintf(vpmu_samples, "\n");
//            fflush(vpmu_samples);

            /* 2. Init for thread profiling. */
            last_EET_ns     = vpmu_estimated_execution_time_ns();
			fprintf(thread_profile, "Starting EET (ns) = %u\n", last_EET_ns);
            fprintf(thread_profile, "PID\tEET\n");
//            fflush(thread_profile);
        }
        else
        {
            /* 1. Sampling routine for general sample file. */
            cur_value = vpmu_hardware_event_select((uint32_t)sample_event_id, (VPMU*) opaque);
    
//            fprintf (stderr, "current value = %u, previous value =%u.\n ", cur_value, previous_value);
            // Check if the sampling interval is reached.
            if ( (cur_value - previous_value) > sampleIntvl )
            {
                // Get the current running thread id.
                cur_pid             = getCurrentPID();
				
				//tianman test
				for(i=0;i<512;i++){
					if(pid_base[i][0]==cur_pid)
						//fprintf(vpmu_samples, "\n pid %d name %s \n", cur_pid, pid_name_base[i]);
						printf("\n pid %d name %s \n", cur_pid, pid_name_base[i]);
				}

                // Dump the Seq_id, PID, and EET.
                fprintf(vpmu_samples, "%u\t%d\t%u", j, cur_pid, vpmu_estimated_execution_time_ns());

                for (i=0; i< sizeofEventIdTable; i++)
                {
//                    DEBUG_PM("EVENT [%d]=%d, on/off(%d), value=", i,
//                                    vpmu_event_id_table[i], isEventMonitored(i) );
                    
                    // Do user want to observe this event?
                    if ( isEventMonitored(i) == EVENT_ON )
                    {
                        fprintf(vpmu_samples, "\t%u", vpmu_hardware_event_select(
                                    (uint32_t)vpmu_event_id_table[i], (VPMU*) opaque));
//                        DEBUG_PM("%lu !.\n", vpmu_hardware_event_select(
//                                    (uint32_t)vpmu_event_id_table[i], (VPMU*) opaque));
                    }
                    else
                    {
//                        DEBUG_PM("%lu.\n", vpmu_hardware_event_select(
//                                    (uint32_t)vpmu_event_id_table[i], (VPMU*) opaque));
                    }
                }
//                DEBUG_PM("\n");
                fprintf(vpmu_samples, "\n");
                previous_value = cur_value;
                j++;
            }
            
            /* 2. Sampling routine for thread profiling. */
            cur_EET_ns = vpmu_estimated_execution_time_ns();
            if ( cur_EET_ns - last_EET_ns > threadSampleIntl ) {
                // Get the current running thread id.
                cur_pid             = getCurrentPID();
                fprintf(thread_profile, "%u\t%u\n", cur_pid, cur_EET_ns-last_EET_ns);
                last_EET_ns = cur_EET_ns;
                k++;
            }
        }
        if ( j % 10000 == 0 )
            fflush(vpmu_samples);
        if ( k % 10000 == 0)
            fflush(thread_profile);
    }

    DEBUG_PM("Sampling finished...\n\n");

    fflush(vpmu_samples);
    fclose (vpmu_samples);
    fflush(thread_profile);
    fclose(thread_profile);
}
#endif

void *vpmu_perf_monitoring_thread (void *opaque)
{
    fprintf(stderr,"VPMU worker thread (performance monitoring) : start waiting signal.\n");

    while (1) 
    {
        pthread_mutex_lock(&mutex_sync_perf_mntr);
        pthread_cond_wait(&perf_mntr_cond, &mutex_sync_perf_mntr);
        pthread_mutex_unlock(&mutex_sync_perf_mntr);

        DEBUG_PM("VPMU sampling thread is signaled, mode: ");

        if (VPMU_enabled) {
            // VPMU is activated; we get meaningful events.
            DEBUG_PM("processor event sampling.\n");
            samplingProcessorEvents(opaque);
        }
        else {
            // VPMU is not in operation.
            DEBUG_PM("peripheral power sampling.\n");
            samplingPeripheralPower(opaque);
        }
    }
    pthread_exit(NULL);
}
