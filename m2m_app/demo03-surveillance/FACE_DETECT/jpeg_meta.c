
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include "sys/mman.h"
#include  <fcntl.h>
int ImageFromFile(char *infilename, unsigned char** buffer)
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
static bool get_jpeg_size(unsigned char* data, unsigned int data_size, unsigned short *width, unsigned short *height) 
{
   //Check for valid JPEG image
   int i=0;   // Keeps track of the position within the file
   if(data[i] == 0xFF && data[i+1] == 0xD8 && data[i+2] == 0xFF && data[i+3] == 0xE0) 
   {
      i += 4;
      // Check for valid JPEG header (null terminated JFIF)
      if(data[i+2] == 'J' && data[i+3] == 'F' && data[i+4] == 'I' && data[i+5] == 'F' && data[i+6] == 0x00) {
         //Retrieve the block length of the first block since the first block will not contain the size of file
         unsigned short block_length = data[i] * 256 + data[i+1];
         while(i<data_size) 
         {
            i+=block_length;               //Increase the file index to get to the next block
            if(i >= data_size) return false;   //Check to protect against segmentation faults
            if(data[i] != 0xFF) return false;   //Check that we are truly at the start of another block
            if(data[i+1] == 0xC0) 
            {            
                //0xFFC0 is the "Start of frame" marker which contains the file size
                //The structure of the 0xFFC0 block is quite simple
                // [0xFFC0][ushort length][uchar precision][ushort x][ushort y]
               *height = data[i+5]*256 + data[i+6];
               *width = data[i+7]*256 + data[i+8];
               return true;
            }
            else
            {
               i+=2;                              //Skip the block marker
               block_length = data[i] * 256 + data[i+1];   //Go to the next block
            }
         }
         return false;                     //If this point is reached then no size was found
      }
      else
            return false; //Not a valid JFIF string
         
   }
   else
        return false; //Not a valid SOI header
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


    if(argc == 2)
    {
        infilename = argv[1];
    }
    else
    {
        fprintf(stderr, "input format error\n");
        exit(0);
    }


    printf("Devcie ID = %d\n", id);
    printf("Input file name = %s\n", infilename);
    printf("Output file name = %s\n", outfilename);

    char get_char;
    int p = 0;
    int SendDevice;
    int SendSize;
    int sum = 0;

    //Read Image from file(camera)
    lSize = ImageFromFile(infilename, &buffer);
    unsigned short width,height;
    bool jpegfile;
    jpegfile =get_jpeg_size(buffer, lSize, &width, &height);
    if(jpegfile)
        printf("width = %d\nheight = %d\n",width,height);
    else
        printf("Not jpeg file\n");
    free(buffer);


    return 0;
}
