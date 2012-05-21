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
    char send_data[] = "I am node three!! \0";
    char recv_data[100] = "";
    int ret;
    ret = Network_Init(id, "Zigbee");

    switch(id)
    {
        case 1:
                printf("Receiving ... \n");
                Network_Recv(recv_data, "Zigbee");
                printf("Receive data: %s\n",recv_data);
                break;
        case 3:
                printf("Sending ... [%s]\n",send_data);
                Network_Send(1, send_data, "Zigbee");
                break;
        default:
                break;
    }

    int inp= 8;
    do{
    scanf("%d",&inp);
    }while(inp);

    ret = Network_Exit("Zigbee");
    return 0;
}
