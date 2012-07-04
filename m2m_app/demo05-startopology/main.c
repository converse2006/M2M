
// C
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>


//M2M Common API
#include "../network_device.c"

int main(int argc, char **argv)
{
    char *infilename, *outfilename;
    FILE *rFile,*wFile;
    int lSize,i,ind = 0,count = 0;
    int flag = 1;
    int range;
    int id;
    int totalnodes;
    int sendtimes;
    int ret;
    unsigned char *buffer = NULL;
    size_t result;
    char netbuff[PACKETSIZE+1] = "2:88715@";
    char pic_size[PACKETSIZE+1];
    char idbuff[PACKETSIZE+1];


    if(argc == 4)
    {
        id = atoi(argv[1]);
        totalnodes = atoi(argv[2]);
        sendtimes = atoi(argv[3]);
    }
    else
    {
        fprintf(stderr, "./star [DeviceID] [TotalNodesNumber] [SendTime]\n");
        exit(0);
    }


    printf("Devcie ID = %d\n", id);
    printf("Total Nodes = %d\n", totalnodes);
    printf("Send Time = %d\n", sendtimes);
    printf("Network Payload: %d\n", PACKETSIZE);

    char get_char;
    int p = 0;
    int SendDevice;
    int SendSize;
    int sum = 0;

    ret = Network_Init(id, "Zigbee", 5);
    if(id == 1) //Coordinator
    {
        int ind;

        for(ind = 1; ind <= sendtimes * (totalnodes - 1); ind++)
        do{
            ret = Network_Recv(netbuff, "Zigbee");
        }while(ret == 0);
    }
    else //End device
    {
        int ind;


        for(ind = 1; ind <= sendtimes; ind++)
        do{
            ret = Network_Send(1, netbuff, "Zigbee");
        }while(ret == 0);
    }
    ret = Network_Exit("Zigbee");

    return 0;
}

