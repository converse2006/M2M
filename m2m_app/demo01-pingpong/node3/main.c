#include "network_device.c"
int main()
{
    char send_data[] = "I am node one!!";
    char recv_data[100];
    int ret;
    ret = Network_Init(3, "Zigbee");
    int inp= 8;
    do{
    scanf("%d",&inp);
    }while(inp);

    /*Network_Send(1, send_data, "Zigbee");
    Network_Recv(recv_data, "Zigbee");
    printf("Receive data: %s\n",recv_data);*/
    ret = Network_Exit("Zigbee");
}