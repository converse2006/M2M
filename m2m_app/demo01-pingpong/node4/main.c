#include "network_device.c"
int main()
{
    char send_data[] = "I am node one!!";
    char recv_data[100];
    int ret;
    ret = Network_Init(4, "Zigbee");
    Network_Send(3, send_data, "Zigbee");
    Network_Recv(recv_data, "Zigbee");
    printf("Receive data: %s\n",recv_data);
    ret = Network_Exit("Zigbee");
}
