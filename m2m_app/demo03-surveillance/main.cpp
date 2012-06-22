// OpenCV
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"

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

#define SCALE         1
#define CASCADE_FILE "haarcascade_frontalface_alt.xml"
using namespace std;

//M2M Common API
#include "../network_device.c"

bool DEBUG;

CvSeq* detect( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade);
void draw(IplImage* img, CvSeq* faces);
void usage();

CvSeq* detect( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade) {
    cvClearMemStorage( storage );
    if ( cascade ){
        return cvHaarDetectObjects( img, cascade, storage,
                                     1.1, 3, CV_HAAR_DO_CANNY_PRUNING,
                                     cvSize(40, 40) );
    }
    return 0;
}

void draw(IplImage* img, CvSeq* faces) {
    CvPoint pt1, pt2;
    for (int i = 0; i < faces->total; i++ ){
        CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
        pt1.x =  r->x            * SCALE;
        pt2.x = (r->x+r->width)  * SCALE;
        pt1.y =  r->y            * SCALE;
        pt2.y = (r->y+r->height) * SCALE;
        cvRectangle( img, pt1, pt2, CV_RGB(255,0,0), 3, 8, 0 );
    }
}

void facedetect(char* infilename, char* outfilename, unsigned char* buffer,unsigned int size)
{
    string imageIn;
    string imageOut;
    string cascade_file = CASCADE_FILE;

    IplImage* image = 0;
    bool in = true;
    DEBUG = false;

    imageOut.assign(outfilename);
    imageIn.assign(infilename);
    cout<<"imageIn  :"<<imageIn<<endl;
    cout<<"imageOut :"<<imageOut<<endl;
    cout<<"cascade  :"<<cascade_file<<endl;
    
    //FIXME
    FILE* wFile;
    wFile = fopen(infilename, "w+b");
    fwrite(buffer, 1, size, wFile);
    fclose(wFile);
    
    
    
     // load the HaarClassifierCascade
    CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*)cvLoad( cascade_file.c_str(), 0, 0, 0 );
    if ( !cascade ) {
        cout<<"Error: Could not load classifier cascade"<<endl;
        exit(0);
    }

    // detect faces
    cout<<"Detect faces"<<endl;
    CvMemStorage* storage = cvCreateMemStorage(0);

    //From file 
    image = cvLoadImage( imageIn.c_str(), -1 );
    //From buffer FIXME This part doesn't work
    //image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 1);
    //cvSetData(image, buffer, width);

    if (image) {
        cout<<"Image exist"<<endl;
        CvSeq* faces = detect(image, storage, cascade);
        if (faces->total != 0) {
            cout<<"Image contain "<<faces->total<<"people!!"<<endl;
            /*draw (image, faces);
            if(imageOut.size() == 0) {
                cvSaveImage(("face_" + imageIn).c_str(), image );
                cout<<"ImageOut: "<< ("face_" + imageIn) <<endl;
            }else{
                cvSaveImage(imageOut.c_str(), image );
                cout<<"ImageOut: "<< imageOut <<endl;
            }*/
        }else{
            cout<<"No faces found."<<endl;
        }
        cvReleaseImage( &image );
    }
    else
        cout<< "Image doesn't exist";

}

char* jpeg_quality(string imageIn, int quality)
{
            IplImage* image = 0;
            char* outfilename = "tmp.jpeg";
            string outfilename2;
            outfilename2.assign(outfilename);
            
            image = cvLoadImage( imageIn.c_str(), -1 );

            int p[3];
            p[0] = CV_IMWRITE_JPEG_QUALITY;
            p[1] = quality;
            p[2] = 0;
            cvSaveImage(outfilename2.c_str(), image, p);
            return outfilename;
            
}

int ImageFromFile(char *infilename, unsigned char** buffer, int quality)
{
        string imageIn;
        string readfilename;
        imageIn.assign(infilename);
        infilename = jpeg_quality(imageIn, quality);

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
    int quality;

    if(argc == 5)
    {
        id = atoi(argv[1]);
        infilename = argv[2];
        outfilename =argv[3];
        quality =atoi(argv[4]);
    }
    else
    {
        fprintf(stderr, "input format error\n");
        exit(0);
    }

    ret = Network_Init(id, "Zigbee", 5);

    printf("Devcie ID = %d\n", id);
    printf("Input file name = %s\n", infilename);
    printf("Output file name = %s\n", outfilename);

    char get_char;
    int p = 0;
    int SendDevice;
    int SendSize;
    int sum = 0;

    if(id == 1) //Coordinator
    {
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
                    ret = Network_Send(2, netbuff, "Zigbee");
                }while(ret == 0);

                ind = 0;
                range = PACKETSIZE;
                if(range > SendSize)
                    range = SendSize;
                while(flag == 1)
                {
                    count++;
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

                facedetect(infilename, outfilename, buffer,SendSize);
                free(buffer);

                /*wFile = fopen(outfilename, "w+b");
                fwrite(buffer, 1, SendSize, wFile);
                fclose(wFile);*/
    }
    else if(id == 2) //End device
    {
                //Read Image from file(camera)
                lSize = ImageFromFile(infilename, &buffer, quality);

                bzero(netbuff, sizeof(netbuff));
                sprintf(pic_size, "%d", lSize);
                sprintf(idbuff, "%d", 2);
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



    ret = Network_Exit("Zigbee");

    return 0;
}

