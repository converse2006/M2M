#include "network_device.c"
int main()
{
    char send_data[] = "I am node one!!";
    char recv_data[100];
    int ret;
    ret = Network_Init(1, "Zigbee");
    printf("Sending ... [%s]\n",send_data);
    Network_Send(3, send_data, "Zigbee");
    int inp= 8;
    do{
    scanf("%d",&inp);
    }while(inp);


    /*while(inp)
    {
        sleep(1);
        inp--;
    }*/
    //sleep(4);
    //Network_Recv(recv_data, "Zigbee");
    //printf("Receive data: %s\n",recv_data);
    ret = Network_Exit("Zigbee");
    return 0;
}