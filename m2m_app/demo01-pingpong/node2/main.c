#include "network_device.c"
int main()
{
    char send_data[100] = "[Node 2] I got data: ";
    char recv_data[100];
    int ret;
    ret = Network_Init(2, "Zigbee");

    int inp= 8;
    do{
    scanf("%d",&inp);
    }while(inp);
    //Network_Recv(recv_data, "Zigbee");
    //printf("Receive data: %s\n",recv_data);
    //strcat(send_data,recv_data);
    //printf("Sending packet... [%s]\n",send_data);
    //Network_Send(1, send_data, "Zigbee");
    ret = Network_Exit("Zigbee");
    return 0;
}
