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

    char tmp_data[] ="I am node three: ";
    char send_data1[100] = "Just Go !!!!!";
    char send_data2[100] = "Chun-Han Chen";
    char send_data3[100] = "Zigbeenetwork";
    char send_data4[100] = "M2M 012345678";
    char recv_data[100] = "";

    int ind,times = 10000;
    int ret;
    ret = Network_Init(id, "Zigbee");
    switch(id)
    {
        case 1:
                for(ind = 0 ; ind < times; ind++)
                {
                    printf("[%d]Receiving ... \n",ind);
                    Network_Recv(recv_data, "Zigbee");
                    printf("[%d]Receive data: %s\n", ind, recv_data);
                    /*if(ind % 2)
                    {
                        printf("[%d]Sending ... [%s]\n", ind, send_data1);
                        Network_Send(2, send_data1, "Zigbee");
                    }
                    else
                    {*/
                        printf("[%d]Sending ... [%s]\n", ind, send_data2);
                        Network_Send(2, send_data2, "Zigbee");
                    //}
                }
                break;
        case 2:
                for(ind = 0 ; ind < times; ind++)
                {
                   /* if(ind % 2)
                    {
                        printf("[%d]Sending ... [%s]\n", ind, send_data3);
                        Network_Send(1, send_data3, "Zigbee");
                    }
                    else
                    {*/
                        printf("[%d]Sending ... [%s]\n", ind, send_data4);
                        Network_Send(1, send_data4, "Zigbee");
                    //}

                    printf("[%d]Receiving ... \n",ind);
                    Network_Recv(recv_data, "Zigbee");
                    printf("[%d]Receive data: %s\n", ind, recv_data);
                }
                break;
        default:
                break;
    }

    printf("Program finish!!\n");
    int inp= 8;
    do{
    scanf("%d",&inp);
    }while(inp);

    ret = Network_Exit("Zigbee");
    return 0;
}
