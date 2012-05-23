#include "network_device.c"
int main(int argc, char* argv[])
{
    int id;
    if(argc != 2)
    {
        fprintf(stderr, "Input parameter error!!\n");
        exit(0);
    }
    else
        id = atoi(argv[1]);
    char send_data[100] = "I am node three!!";
    char recv_data[100] = "";
    int ret;
    ret = Network_Init(id, "Zigbee");
    int ind,times = 10000;
    switch(id)
    {
        case 1:
                for(ind = 0 ; ind < times; ind++)
                {
                    printf("[%d]Receiving ... \n",ind);
                    Network_Recv(recv_data, "Zigbee");
                    printf("[%d]Receive data: %s\n", ind, recv_data);
                }
                break;
        case 3:
                for(ind = 0 ; ind < times; ind++)
                {
                    printf("[%d]Sending ... [%s]\n", ind, send_data);
                    Network_Send(1, send_data, "Zigbee");
                }
                break;
        default:
                break;
    }

    if(id == 2 || id == 1)
    {
    int inp= 8;
    do{
    scanf("%d",&inp);
    }while(inp);
    }

    ret = Network_Exit("Zigbee");
    printf("Program finish!!\n");
    return 0;
}
