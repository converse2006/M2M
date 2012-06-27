// C++
#include <iostream>
#include <string>
#include <utility>

// C
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

using namespace std;

//M2M Common API
#include "../network_device.c"



int ImageFromFile(char *infilename,unsigned char** buffer)
{
        int lSize;
        FILE* rFile;
        size_t result;

        rFile = fopen(infilename,"rb");
        if( rFile == NULL)
        {
            printf("file open error,no such file!!\n");
            exit(1);
        }

        fseek(rFile, 0, SEEK_END);
        lSize = ftell(rFile);
        rewind(rFile);
        printf("file size = %d\n",lSize);

        *buffer = (unsigned char*) malloc(sizeof(unsigned char)*lSize);
        if(*buffer == NULL){fprintf(stderr, "Memory error"); exit(2);}
        result = fread(*buffer, 1, lSize, rFile);
        if(result != lSize){fprintf(stderr, "Reading error"); exit(3);}

        fclose(rFile);
        return lSize;

}


int main(int argc, char **argv)
{
    char *infilename, *outfilename;
    FILE *rFile,*wFile;
    int lSize,i,ind = 0,count = 0;
    int flag = 1;
    int range;
    int id;
    int ret;
    unsigned char *buffer = NULL;
    size_t result;
    char netbuff[PACKETSIZE+1] = "2:88715@";
    char pic_size[PACKETSIZE+1];
    char idbuff[PACKETSIZE+1];


    if(argc == 4)
    {
        infilename = argv[2];
        outfilename =argv[3];
        id = atoi(argv[1]);
    }
    else
    {
        fprintf(stderr, "input format error\n");
        exit(0);
    }


    printf("Devcie ID = %d\n", id);
    printf("Input file name = %s\n", infilename);
    printf("Output file name = %s\n", outfilename);
    printf("Network Configure:\n");
    printf("Network Payload: %d\n", PACKETSIZE);

    char get_char;
    int p = 0;
    int SendDevice;
    int SendSize;
    int sum = 0;

    if(id == 1) //Coordinator
    {
                ret = Network_Init(id, "Zigbee", 5);
                //Read Image from file(camera)
                //SendSize = ImageFromFile(infilename, &buffer);

                ret = Network_Recv(netbuff, "Zigbee");
                printf("netbuff content:[%s]\n",netbuff);
                while(netbuff[ind] != '@')
                {
                    switch(p)
                    {
                        case 0:
                                while(netbuff[ind] != ':')
                                {
                                    sum = sum *10 +(netbuff[ind] - '0');
                                    ind++;
                                }
                                SendDevice = sum;
                                sum = 0;
                                ind++;
                                p++;
                                break;
                        case 1: 
                                while(netbuff[ind] != '@')
                                {
                                    sum = sum *10 +(netbuff[ind] - '0');
                                    ind++;
                                }
                                SendSize = sum;
                    }

                }

                printf("SendDevice = %d\n SendSize = %d\n", SendDevice, SendSize);
                buffer = (unsigned char*) malloc(sizeof(unsigned char)*SendSize);
                bzero(netbuff, PACKETSIZE);
                strcpy(netbuff,"OK");

                do{
                    ret = Network_Send(SendDevice, netbuff, "Zigbee");
                }while(ret == 0);

                ind = 0;
                range = PACKETSIZE;
                if(range > SendSize)
                    range = SendSize;
                while(flag == 1)
                {
                    count++;
                    //printf("Packet Count:%d\n",count);
                    ret = Network_Recv(netbuff, "Zigbee");
                    for(i = 0; i < (range - ind); i++)
                      buffer[ind + i] = netbuff[i];

                    if(ind < SendSize)
                    {
                        if((range + PACKETSIZE) > SendSize)
                        { ind = range; range = SendSize; }
                        else
                        { ind = range; range += PACKETSIZE; }
                    }

                    if(ind == SendSize)
                          flag = 0;
                }


                wFile = fopen(outfilename, "w+b");
                fwrite(buffer, 1, SendSize, wFile);
                fclose(wFile);
                free(buffer);
    }
    else if(id == 6) //End device
    {
                ret = Network_Init(id, "Zigbee", 5);
                //Read Image from file(camera)
                lSize = ImageFromFile(infilename, &buffer);

                bzero(netbuff, sizeof(netbuff));
                sprintf(pic_size, "%d", lSize);
                sprintf(idbuff, "%d", id);
                strcpy(netbuff,idbuff);
                strcat(netbuff,":");
                strcat(netbuff,pic_size);
                strcat(netbuff,"@");
                printf("Packet Content:[%s]\n",netbuff);
                do{
                    ret = Network_Send(1, netbuff, "Zigbee");
                }while(ret == 0);
                ret = Network_Recv(netbuff, "Zigbee");
                printf("Receive_data = %s\n", netbuff);

                ind = 0;
                range = PACKETSIZE;
                if(range > lSize)
                    range = lSize;
                while(flag == 1)
                {
                    count++;
                    bzero(netbuff, PACKETSIZE);
                    for(i = 0; i < (range - ind); i++)
                        netbuff[i] = buffer[ind + i];

                    do{
                        //printf("Packet Count:%d\n",count);
                        ret = Network_Send(1, netbuff, "Zigbee");
                    }while(ret == 0);

                    if(ind < lSize)
                    {
                        if((range + PACKETSIZE) > lSize)
                        { ind = range; range = lSize; }
                        else
                        { ind = range; range += PACKETSIZE; }
                    }
                    
                    if(ind == lSize)
                          flag = 0;
                }

                free(buffer);
    }
    else
    {
        ret = Network_Init(id, "Zigbee", 0);
        int inp=1;
        while(inp)
            scanf("%d",&inp);
    }



    ret = Network_Exit("Zigbee");

    return 0;
}

