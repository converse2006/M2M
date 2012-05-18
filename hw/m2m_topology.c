#include "m2m.h"
#include "m2m_internal.h"
#include "m2m_mm.h"

int TotalNetworkTypeDevice[3]= {0};
int BeforeNetworkTypeDevice[3] = {0};
unsigned int NODE_MAP[MAX_NODE_NUM][MAX_NODE_NUM];
unsigned int NODE_LOCATION[MAX_NODE_NUM][3];
char NODE_TYPE[MAX_NODE_NUM][4];
long NODE_SHM_LOCATION[MAX_NODE_NUM] = {0};

extern long shm_address;
extern VND GlobalVND;
static M2M_ERR_T node_infofetch(int DeviceID);

void show_m2m_position();
void show_m2m_map();
M2M_ERR_T m2m_topology_init(int DeviceID)
{
    int rc = M2M_SUCCESS;
    fprintf(stderr,"m2m_topology!!\n");
    int ind;
    for(ind = 0; ind < 3; ind++)
    {
        TotalNetworkTypeDevice[ind]= 0;
        BeforeNetworkTypeDevice[ind] = 0;
    }

    rc = node_infofetch(DeviceID);

#ifdef DEBUG_M2M
    if(rc == M2M_SUCCESS)
    {
        show_m2m_map();
        show_m2m_position();
    }
#endif
    return rc;
}



//  =====   INTERNAL FUNCTIONS  =======
static M2M_ERR_T node_infofetch(int DeviceID)
{
    FILE *pFile;
    char info[40];
    long shm_location = 0;
    int index;
    GlobalVND.DeviceID = DeviceID;
    pFile = fopen("external/qemu-paslab/m2m_app/topology.conf","r");
    if(pFile == NULL) {fputs("File topology.conf open error",stderr); exit(0); return M2M_ERROR; }
    fgets(info, 40, pFile); //Fetch TotalDevice Number
    GlobalVND.TotalDeviceNum = atoi(info);
    if(DeviceID > GlobalVND.TotalDeviceNum) {fputs("DeviceID larger than Device number!",stderr); return M2M_ERROR; }
    for(index = 1; index <= GlobalVND.TotalDeviceNum; index++)
    {
        NODE_SHM_LOCATION[index] = shm_location;
        if(index != DeviceID)
        {
            int ind = 0, select = 0;
            fgets(info, 40, pFile);
            switch(info[1]) //Counting each zigbee device type
            {
                case 'C': 
                            TotalNetworkTypeDevice[0]++;
                            shm_location += COORDINATOR_SIZE;
                            strcpy(NODE_TYPE[index], "ZC");
                            break;
                case 'R': 
                            TotalNetworkTypeDevice[1]++;
                            shm_location += ROUTER_SIZE;
                            strcpy(NODE_TYPE[index], "ZR");
                            break;
                case 'E': 
                            TotalNetworkTypeDevice[2]++;
                            shm_location += END_DEVICE_SIZE;
                            strcpy(NODE_TYPE[index], "ZED");
                            break;
            }
            while(ind < strlen(info))
            {

                if(info[ind] == '{')
                {
                    if(select == 0) //first {} for device location
                    {
                        ind++;
                        int dim = 0;
                        while(info[ind] != '}' && ind < strlen(info))
                        {
                            int sum = 0, flag = 0 ;
                            while(info[ind] <= '9' && info[ind] >= '0')
                            {
                                flag = 1;
                                sum = sum * 10 + (info[ind] - '0');
                                ind++;
                            }
                            if(flag)
                            {
                                NODE_LOCATION[index][dim] = sum;
                                dim++;
                            }
                            else
                                ind++;
                        }
                        select++;
                    }
                    else //second {} for link between device 
                    {
                        ind++;
                        while(info[ind] != '}' && ind < strlen(info))
                        {
                            int sum = 0, flag = 0 ;

                            while(info[ind] <= '9' && info[ind] >= '0')
                            {
                                flag = 1;
                                sum = sum * 10 + (info[ind] - '0');
                                ind++;
                            }
                            if(flag)
                            {
                                NODE_MAP[index][sum] = 1;
                                NODE_MAP[sum][index] = 1;
                            }
                            else
                                ind++;
                        }

                    }
                }
                ind++;
            }
        }
        else if( fgets(info, 40, pFile) != NULL)
        {
            int lens = strlen(info);
            int count = 0, select = 0;
            
           //Record how much device before me, it's uesd to caculate metadata related location
           BeforeNetworkTypeDevice[0] = TotalNetworkTypeDevice[0];
           BeforeNetworkTypeDevice[1] = TotalNetworkTypeDevice[1];
           BeforeNetworkTypeDevice[2] = TotalNetworkTypeDevice[2];
            
            switch(info[1]) //Counting each zigbee device type
            {
                case 'C': 
                            TotalNetworkTypeDevice[0]++;
                            shm_location += COORDINATOR_SIZE;
                            strcpy(NODE_TYPE[index], "ZC");
                            break;
                case 'R': 
                            TotalNetworkTypeDevice[1]++;
                            shm_location += ROUTER_SIZE;
                            strcpy(NODE_TYPE[index], "ZR");
                            break;
                case 'E': 
                            TotalNetworkTypeDevice[2]++;
                            shm_location += END_DEVICE_SIZE;
                            strcpy(NODE_TYPE[index], "ZED");
                            break;
            }

            while(count < lens)
            {
                int sum = 0, linknum = 0, flag = 1, ind;
                if(info[count] != ' ')
                    switch(select)
                    {
                        case 0:  //Node 
                            GlobalVND.DeviceType[0] = info[count];
                            GlobalVND.DeviceType[1] = info[count + 1];
                            if(info[count+2] != ' ') 
                            {   
                                GlobalVND.DeviceType[2] = info[count + 2];
                                count++;
                            }
                            count += 3;
                            select ++;
                         break;
                        case 1:
                            for(ind = 0; ind < 3; ind++)
                            {
                                while(info[count] <= '9' && info[count] >= '0')
                                {
                                    sum = sum * 10 + (info[count] - '0');
                                    count++;
                                }
                                GlobalVND.Position[ind] = sum;
                                NODE_LOCATION[index][ind] = sum;
                                if(ind != 2)
                                    count++;
                                else 
                                    count += 2;
                                sum = 0;
                            }
                            select++;
                        break;
                        case 2:
                            while(flag)
                            {
                                sum = 0;
                                while(info[count] <= '9' && info[count] >= '0')
                                {
                                    sum = sum * 10 + (info[count] - '0');
                                    count++;
                                }
                                GlobalVND.Neighbors[linknum] = sum;
                                linknum++;
                                if(info[count] == ',')
                                    count++;
                                else
                                {
                                    flag = 0;
                                    GlobalVND.NeighborNum = linknum;
                                }
                            }
                            select++;
                            break;
                    }
                count++;
            }
            printf("Total Device Number = %d\n", GlobalVND.TotalDeviceNum);
            printf("Zigbee device id = %d\n", GlobalVND.DeviceID);
            printf("Zigbee device role = %s\n", GlobalVND.DeviceType);
            printf("Zigbee location = {%4d, %4d, %4d}\n", GlobalVND.Position[0], GlobalVND.Position[1], GlobalVND.Position[2]);
            printf("Zigbee device link node (Toal %d links) = ", GlobalVND.NeighborNum);
            int start;
            for(start=0; start <  GlobalVND.NeighborNum; start++)
                printf("%2d ",GlobalVND.Neighbors[start]);
            printf("\n\n");
        }
    }
            printf("Total Zigbee Coordinator number = %d\n",  TotalNetworkTypeDevice[0]);
            printf("Total Zigbee Router number = %d\n", TotalNetworkTypeDevice[1]);
            printf("Total Zigbee End device number = %d\n", TotalNetworkTypeDevice[2]);
            printf("%d ZC before me\n", BeforeNetworkTypeDevice[0]);
            printf("%d ZR before me\n", BeforeNetworkTypeDevice[1]);
            printf("%d ZED before me\n", BeforeNetworkTypeDevice[2]);
            int start;
            for(start = 1; start <= GlobalVND.TotalDeviceNum ; start++)
                printf("[%d][%s] shm.addr= %ld\n",  start, NODE_TYPE[start], NODE_SHM_LOCATION[start]);
    fclose(pFile);
    return M2M_SUCCESS;
}

#ifdef DEBUG_M2M
//Check Link between device
void show_m2m_map()
{
    int x, y;
    printf("\nNode link\n");
    printf("    ");
    for(x = 1; x <= GlobalVND.TotalDeviceNum; x++)
    printf("%3d ",x);
    printf("\n");
    for(x = 1; x <= GlobalVND.TotalDeviceNum; x++)
    {
        printf("%3d ",x);
        for(y = 1; y <= GlobalVND.TotalDeviceNum; y++)
            printf("%3d ",NODE_MAP[x][y]);
        printf("\n");
    }
}
//Check Device Position 
void show_m2m_position()
{
    int x, y;
    printf("\nNode Position\n");
    for(x = 1; x <= GlobalVND.TotalDeviceNum; x++)
    {
        printf("%3d: ",x);
        for(y = 0; y < 3; y++)
            printf("%3d ",NODE_LOCATION[x][y]);
        printf("\n");
    }
    printf("\n");
}
#endif
